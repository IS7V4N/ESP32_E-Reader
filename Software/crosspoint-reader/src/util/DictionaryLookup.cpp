#include "DictionaryLookup.h"

#include <Logging.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace {
constexpr char MAGIC[4] = {'E', 'H', 'D', 'C'};
constexpr uint16_t SUPPORTED_VERSION = 1;

bool isLetter(const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool isVowel(const char c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }

bool endsWith(const std::string& s, const char* suffix) {
  const size_t n = strlen(suffix);
  return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

// Candidate base forms for a word that is not in the dictionary as written (running -> run, tries -> try, ...).
// The dictionary already contains most inflected forms, this only covers what the source data missed.
void addStemCandidates(const std::string& w, std::vector<std::string>& out) {
  const size_t n = w.size();
  auto add = [&out](std::string s) {
    if (s.size() >= 2 && std::find(out.begin(), out.end(), s) == out.end()) out.push_back(std::move(s));
  };
  // "stopped" / "running": the consonant was doubled before the suffix
  auto addUndoubled = [&](const std::string& stem) {
    if (stem.size() >= 3 && stem[stem.size() - 1] == stem[stem.size() - 2] && !isVowel(stem.back())) {
      add(stem.substr(0, stem.size() - 1));
    }
  };

  if (endsWith(w, "ies") && n > 4) {
    add(w.substr(0, n - 3) + "y");
  } else if (endsWith(w, "es") && n > 3) {
    add(w.substr(0, n - 2));
    add(w.substr(0, n - 1));
  } else if (endsWith(w, "s") && !endsWith(w, "ss") && n > 3) {
    add(w.substr(0, n - 1));
  }

  if (endsWith(w, "ied") && n > 4) {
    add(w.substr(0, n - 3) + "y");
  } else if (endsWith(w, "ed") && n > 3) {
    add(w.substr(0, n - 2));
    add(w.substr(0, n - 1));
    addUndoubled(w.substr(0, n - 2));
  }

  if (endsWith(w, "ing") && n > 4) {
    const std::string stem = w.substr(0, n - 3);
    add(stem);
    add(stem + "e");
    addUndoubled(stem);
  }

  if (endsWith(w, "ily") && n > 4) {
    add(w.substr(0, n - 3) + "y");
  } else if (endsWith(w, "ly") && n > 3) {
    add(w.substr(0, n - 2));
  }

  if (endsWith(w, "est") && n > 4) {
    const std::string stem = w.substr(0, n - 3);
    add(stem);
    add(stem + "e");
    addUndoubled(stem);
  } else if (endsWith(w, "er") && n > 3) {
    const std::string stem = w.substr(0, n - 2);
    add(stem);
    add(stem + "e");
    addUndoubled(stem);
  }
}
}  // namespace

bool DictionaryLookup::open() {
  if (opened) return true;

  if (!Storage.openFileForRead("DICT", DICT_PATH, file)) {
    return false;
  }

  uint8_t header[HEADER_SIZE];
  if (file.read(header, HEADER_SIZE) != static_cast<int>(HEADER_SIZE) || memcmp(header, MAGIC, sizeof(MAGIC)) != 0) {
    LOG_ERR("DICT", "Not a dictionary file: %s", DICT_PATH);
    file.close();
    return false;
  }

  uint16_t version;
  memcpy(&version, header + 4, sizeof(version));
  memcpy(&keyLen, header + 6, sizeof(keyLen));
  memcpy(&count, header + 8, sizeof(count));
  memcpy(&indexOffset, header + 12, sizeof(indexOffset));
  memcpy(&dataOffset, header + 16, sizeof(dataOffset));

  if (version != SUPPORTED_VERSION || keyLen == 0 || keyLen > MAX_KEY_LEN) {
    LOG_ERR("DICT", "Unsupported dictionary version %u / key length %u", version, keyLen);
    file.close();
    return false;
  }

  opened = true;
  LOG_DBG("DICT", "Dictionary opened: %lu words", static_cast<unsigned long>(count));
  return true;
}

void DictionaryLookup::close() {
  if (opened) {
    file.close();
    opened = false;
  }
}

std::string DictionaryLookup::cleanWord(const std::string& raw) {
  std::string out;
  for (size_t i = 0; i < raw.size();) {
    const auto c = static_cast<uint8_t>(raw[i]);

    if (c < 0x80) {
      if (isLetter(static_cast<char>(c))) {
        out.push_back(static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c));
      } else if ((c == '\'' || c == '-') && !out.empty()) {
        out.push_back(static_cast<char>(c));
      } else if (!out.empty()) {
        break;  // punctuation, digit or space after the word
      }
      i++;
      continue;
    }

    // UTF-8 sequences
    if (c == 0xE2 && i + 2 < raw.size() && static_cast<uint8_t>(raw[i + 1]) == 0x80) {
      const auto c3 = static_cast<uint8_t>(raw[i + 2]);
      if (c3 == 0x99 && !out.empty()) {  // typographic apostrophe
        out.push_back('\'');
        i += 3;
        continue;
      }
      if (out.empty()) {  // leading em space, quotes, dashes
        i += 3;
        continue;
      }
      break;
    }
    if (c == 0xC2 && i + 1 < raw.size() && static_cast<uint8_t>(raw[i + 1]) == 0xAD) {  // soft hyphen
      i += 2;
      continue;
    }

    if (!out.empty()) {
      return "";  // accented or non-latin letter inside the word, not English
    }
    // Skip one leading non-ASCII character
    i += (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : 2;
  }

  while (!out.empty() && (out.back() == '\'' || out.back() == '-')) out.pop_back();
  return out;
}

bool DictionaryLookup::find(const std::string& key, std::string& translation) {
  if (key.empty() || key.size() > keyLen) return false;

  uint8_t target[MAX_KEY_LEN] = {};
  memcpy(target, key.data(), key.size());

  const size_t recordSize = keyLen + sizeof(uint32_t);
  int64_t lo = 0;
  int64_t hi = static_cast<int64_t>(count) - 1;
  uint8_t record[MAX_KEY_LEN + sizeof(uint32_t)];

  while (lo <= hi) {
    const int64_t mid = (lo + hi) / 2;
    if (!file.seekSet(indexOffset + static_cast<size_t>(mid) * recordSize) ||
        file.read(record, recordSize) != static_cast<int>(recordSize)) {
      LOG_ERR("DICT", "Index read failed");
      return false;
    }

    const int cmp = memcmp(record, target, keyLen);
    if (cmp == 0) {
      uint32_t offset;
      memcpy(&offset, record + keyLen, sizeof(offset));
      uint8_t len;
      if (!file.seekSet(dataOffset + offset) || file.read(&len, 1) != 1) return false;
      translation.resize(len);
      if (file.read(&translation[0], len) != len) {
        translation.clear();
        return false;
      }
      return true;
    }
    if (cmp < 0) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return false;
}

DictionaryLookup::Result DictionaryLookup::lookup(const std::string& rawWord, std::string& translation) {
  if (!open()) return Result::NoDictionary;

  const std::string word = cleanWord(rawWord);
  if (word.empty()) return Result::NotFound;

  std::vector<std::string> candidates;
  candidates.push_back(word);
  std::string base = word;
  if (endsWith(base, "'s")) {  // possessive
    base.resize(base.size() - 2);
    candidates.push_back(base);
  }
  addStemCandidates(base, candidates);

  for (const auto& candidate : candidates) {
    if (find(candidate, translation)) return Result::Found;
  }
  return Result::NotFound;
}
