#!/usr/bin/env python3
"""
Build the English->Hungarian dictionary file used by the on-device word lookup.

Sources
  * Wiktionary (via kaikki.org / wiktextract JSONL): primary. Translations are ordered by sense, so the most common
    meaning comes first, and inflected forms link back to their lemma (ran -> run).
  * FreeDict eng-hun (dictd): fallback for words Wiktionary has no Hungarian translation for.

Usage (two steps, the first one is slow because the dump is 3+ GB, it is streamed and never stored):

  curl -sSL https://kaikki.org/dictionary/English/kaikki.org-dictionary-English.jsonl \\
      | python3 scripts/build_dict.py extract - > wikt-compact.jsonl

  python3 scripts/build_dict.py build wikt-compact.jsonl --freedict path/to/eng-hun -o en-hu.dic

  python3 scripts/build_dict.py lookup en-hu.dic run ran went children      # sanity check

Copy the result to the SD card as /dict/en-hu.dic

File format (little endian), read by src/util/DictionaryLookup.cpp
  header  (32 bytes): "EHDC", u16 version(1), u16 keyLen(24), u32 count, u32 indexOffset, u32 dataOffset, 12 reserved
  index   (count * (keyLen + 4)): key (lowercase ASCII, NUL padded, sorted bytewise) + u32 offset into data
  data    : u8 length + UTF-8 translation text; several keys may share one offset

Licenses: Wiktionary content is CC BY-SA, FreeDict eng-hun is GPL-2.0+. Keep the attribution in the README.
"""

import argparse
import gzip
import json
import re
import struct
import sys
from collections import OrderedDict

MAGIC = b"EHDC"
VERSION = 1
KEY_LEN = 24
HEADER_SIZE = 32

MAX_PER_POS = 3
MAX_POS_GROUPS = 2
MAX_DEF_BYTES = 110

KEY_RE = re.compile(r"^[a-z][a-z'\-]*$")

# FreeDict is Latin-2 text that was decoded as Latin-1 in places: "legelô" -> "legelő"
FREEDICT_FIXES = str.maketrans({"ô": "ő", "Ô": "Ő", "û": "ű", "Û": "Ű"})


def normalize_key(word):
    w = word.replace("’", "'").strip()
    return w.lower()


def valid_key(key):
    return bool(KEY_RE.match(key)) and len(key.encode("utf-8")) <= KEY_LEN


# --------------------------------------------------------------------------------------------------------------
# extract: kaikki JSONL stream -> compact JSONL
# --------------------------------------------------------------------------------------------------------------


def open_stream(path):
    if path == "-":
        return sys.stdin.buffer
    if path.endswith(".gz"):
        return gzip.open(path, "rb")
    return open(path, "rb")


def cmd_extract(args):
    kept = 0
    total = 0
    for raw in open_stream(args.input):
        total += 1
        # Cheap pre-filter, most entries have neither Hungarian translations nor form_of/alt_of links.
        if b'"lang_code": "hu"' not in raw and b'"form_of"' not in raw and b'"alt_of"' not in raw:
            continue
        try:
            e = json.loads(raw)
        except ValueError:
            continue
        word = e.get("word")
        if not word or e.get("lang_code", "en") != "en":
            continue

        # Every Hungarian translation is emitted as [word, senseIndex, tableIndex]:
        #   senseIndex  index of the sense it belongs to (senses are ordered by importance), -1 if unknown
        #   tableIndex  order of the translation table it came from (the first table is the main meaning)
        # build then ranks by senseIndex, or tableIndex where the sense is unknown.
        senses = e.get("senses", [])
        gloss_to_sense = {}
        for i, s in enumerate(senses):
            for g in s.get("glosses", []) or []:
                gloss_to_sense.setdefault(g, i)

        hu = []
        for i, s in enumerate(senses):
            for t in s.get("translations") or []:
                if t.get("lang_code") == "hu" and t.get("word"):
                    hu.append([t["word"], i, -1])
        tables = {}
        for t in e.get("translations") or []:
            if t.get("lang_code") == "hu" and t.get("word"):
                gloss = t.get("sense", "")
                table = tables.setdefault(gloss, len(tables))
                hu.append([t["word"], gloss_to_sense.get(gloss, -1), table])

        lemmas = []
        for s in e.get("senses", []):
            for k in ("form_of", "alt_of"):
                for link in s.get(k, []) or []:
                    lw = link.get("word")
                    if lw and lw not in lemmas:
                        lemmas.append(lw)

        if not hu and not lemmas:
            continue
        rec = {"w": word, "p": e.get("pos", "")}
        if hu:
            rec["hu"] = hu
        if lemmas:
            rec["fo"] = lemmas
        sys.stdout.write(json.dumps(rec, ensure_ascii=False) + "\n")
        kept += 1
    print(f"extract: {kept} of {total} entries kept", file=sys.stderr)


# --------------------------------------------------------------------------------------------------------------
# build: compact JSONL (+ FreeDict) -> .dic
# --------------------------------------------------------------------------------------------------------------


def unique(seq):
    return list(OrderedDict.fromkeys(seq))


def clean_translations(items):
    """items: [(word, rank)] -> words ordered by rank (stable), without affixes, markup and duplicates."""
    words = []
    for word, _ in sorted(items, key=lambda it: it[1]):
        w = word.strip()
        # skip affixes such as "-ul", unresolved markup and parenthetical-only glosses like "(fent) említett"
        if not w or w.startswith(("-", "(")) or w.endswith("-") or "[" in w:
            continue
        words.append(w)
    return unique(words)


def format_definition(groups):
    """groups: list of (pos, [(translation, rank)]) in source order -> short text such as 'fut, szalad; futás'."""
    by_pos = OrderedDict()
    for pos, items in groups:
        by_pos.setdefault(pos, []).extend(items)
    parts = []
    for items in by_pos.values():
        tr = clean_translations(items)[:MAX_PER_POS]
        if tr:
            parts.append(", ".join(tr))
        if len(parts) >= MAX_POS_GROUPS:
            break
    return truncate("; ".join(parts))


def truncate(text):
    data = text.encode("utf-8")
    if len(data) <= MAX_DEF_BYTES:
        return text
    cut = data[:MAX_DEF_BYTES].decode("utf-8", errors="ignore")
    for sep in ("; ", ", "):
        i = cut.rfind(sep)
        if i > 0:
            return cut[:i]
    return cut


def load_wiktionary(path):
    """Returns (defs, lemma_links): defs[key] -> [(pos, [hu...])], lemma_links[key] -> [lemma keys]."""
    defs = {}
    links = {}
    exact_lower = set()
    with open(path, encoding="utf-8") as f:
        for line in f:
            r = json.loads(line)
            key = normalize_key(r["w"])
            if not valid_key(key):
                continue
            is_lower = r["w"] == key
            if "hu" in r:
                # A lowercase headword beats a capitalised one (may vs. May)
                if is_lower and key not in exact_lower:
                    defs.pop(key, None)
                    exact_lower.add(key)
                if is_lower or key not in exact_lower:
                    # rank: index of the sense the translation belongs to, else the order of its translation table
                    items = [(w, sense if sense >= 0 else max(table, 0)) for w, sense, table in r["hu"]]
                    defs.setdefault(key, []).append((r["p"], items))
            for lemma in r.get("fo", []):
                lk = normalize_key(lemma)
                if valid_key(lk) and lk != key:
                    links.setdefault(key, [])
                    if lk not in links[key]:
                        links[key].append(lk)
    return defs, links


def read_freedict(directory):
    """Returns dict key -> [translations] from a dictd directory (eng-hun.index + eng-hun.dict[.dz])."""
    import os

    b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

    def dec(s):
        n = 0
        for c in s:
            n = n * 64 + b64.index(c)
        return n

    dz = os.path.join(directory, "eng-hun.dict.dz")
    plain = os.path.join(directory, "eng-hun.dict")
    data = gzip.open(dz).read() if os.path.exists(dz) else open(plain, "rb").read()
    result = {}
    with open(os.path.join(directory, "eng-hun.index"), encoding="utf-8") as f:
        for line in f:
            w, o, n = line.rstrip("\n").split("\t")
            if w.startswith("00database"):
                continue
            key = normalize_key(w)
            if not valid_key(key) or key in result:
                continue
            text = data[dec(o) : dec(o) + dec(n)].decode("utf-8", errors="replace").translate(FREEDICT_FIXES)
            lines = text.split("\n")[1:]  # first line is "headword /ipa/"
            tr = []
            for ln in lines:
                ln = re.sub(r"^\d+\.\s*", "", ln).strip()
                if ln:
                    tr.append(ln)
            if tr:
                result[key] = tr
    return result


def cmd_build(args):
    defs, links = load_wiktionary(args.input)
    print(f"wiktionary: {len(defs)} headwords with translations, {len(links)} inflected/alternative forms",
          file=sys.stderr)

    texts = {}  # key -> definition text
    for key, groups in defs.items():
        t = format_definition(groups)
        if t:
            texts[key] = t

    # Inflected forms share the definition of their lemma (follow the chain a few hops)
    resolved = 0
    for key in links:
        if key in texts:
            continue
        frontier = [key]
        for _ in range(3):
            nxt = []
            for k in frontier:
                for lk in links.get(k, []):
                    if lk in texts:
                        texts[key] = texts[lk]
                        resolved += 1
                        break
                    nxt.append(lk)
                if key in texts:
                    break
            if key in texts:
                break
            frontier = nxt
    print(f"forms resolved to a lemma: {resolved}", file=sys.stderr)

    if args.freedict:
        fd = read_freedict(args.freedict)
        added = 0
        for key, tr in fd.items():
            if key not in texts:
                t = format_definition([("", [(w, 0) for w in tr])])
                if t:
                    texts[key] = t
                    added += 1
        print(f"freedict fallback: {added} added", file=sys.stderr)

    keys = sorted(texts, key=lambda k: k.encode("utf-8"))

    # data section: identical texts stored once
    offsets = {}
    data = bytearray()
    for k in keys:
        t = texts[k]
        if t not in offsets:
            b = t.encode("utf-8")
            offsets[t] = len(data)
            data += bytes([len(b)]) + b

    index_offset = HEADER_SIZE
    data_offset = index_offset + len(keys) * (KEY_LEN + 4)
    with open(args.output, "wb") as out:
        out.write(MAGIC + struct.pack("<HHIII", VERSION, KEY_LEN, len(keys), index_offset, data_offset) + b"\0" * 12)
        for k in keys:
            out.write(k.encode("utf-8").ljust(KEY_LEN, b"\0") + struct.pack("<I", offsets[texts[k]]))
        out.write(bytes(data))
    print(f"wrote {args.output}: {len(keys)} keys, {len(offsets)} definitions, "
          f"{data_offset + len(data)} bytes", file=sys.stderr)


# --------------------------------------------------------------------------------------------------------------
# lookup: reference implementation of the on-device binary search, for checking the output
# --------------------------------------------------------------------------------------------------------------


def lookup(f, word):
    f.seek(0)
    hdr = f.read(HEADER_SIZE)
    magic, version, key_len, count, idx_off, data_off = struct.unpack("<4sHHIII", hdr[:20])
    assert magic == MAGIC and version == VERSION
    rec = key_len + 4
    target = word.encode("utf-8")
    lo, hi = 0, count - 1
    while lo <= hi:
        mid = (lo + hi) // 2
        f.seek(idx_off + mid * rec)
        r = f.read(rec)
        k = r[:key_len].rstrip(b"\0")
        if k == target:
            (off,) = struct.unpack("<I", r[key_len:])
            f.seek(data_off + off)
            n = f.read(1)[0]
            return f.read(n).decode("utf-8")
        if k < target:
            lo = mid + 1
        else:
            hi = mid - 1
    return None


def cmd_lookup(args):
    with open(args.dic, "rb") as f:
        for w in args.words:
            print(f"{w}: {lookup(f, normalize_key(w))}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("extract", help="kaikki JSONL (path or - for stdin) -> compact JSONL on stdout")
    p.add_argument("input")
    p.set_defaults(fn=cmd_extract)

    p = sub.add_parser("build", help="compact JSONL -> .dic")
    p.add_argument("input")
    p.add_argument("--freedict", help="directory with eng-hun.index and eng-hun.dict[.dz]")
    p.add_argument("-o", "--output", default="en-hu.dic")
    p.set_defaults(fn=cmd_build)

    p = sub.add_parser("lookup", help="look words up in a built .dic")
    p.add_argument("dic")
    p.add_argument("words", nargs="+")
    p.set_defaults(fn=cmd_lookup)

    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
