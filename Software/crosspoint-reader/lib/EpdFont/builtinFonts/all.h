#pragma once

// ── Mindig betöltött fontok ───────────────────────────────────────────────────
// Ezek a main.cpp-ben az #ifndef OMIT_FONTS blokkon KÍVÜL vannak (39-44, 116-125. sorok)
// tehát mindig kellnek, minden build-ben.

// bookerly 14 - az alapértelmezett olvasási font
#include <builtinFonts/bookerly_14_bold.h>
#include <builtinFonts/bookerly_14_bolditalic.h>
#include <builtinFonts/bookerly_14_italic.h>
#include <builtinFonts/bookerly_14_regular.h>

// UI és rendszer fontok
#include <builtinFonts/notosans_8_regular.h>
#include <builtinFonts/ubuntu_10_bold.h>
#include <builtinFonts/ubuntu_10_regular.h>
#include <builtinFonts/ubuntu_12_bold.h>
#include <builtinFonts/ubuntu_12_regular.h>

// ── Teljes fontcsomag (csak ha OMIT_FONTS nincs define-olva) ──────────────────
// Normál build-ben (default, gh_release) ez mind betöltődik.
// 4MB build-ben (-DOMIT_FONTS=1) ez az egész blokk kimarad a fordításból.
#if !defined(OMIT_FONTS)

#include <builtinFonts/bookerly_12_bold.h>
#include <builtinFonts/bookerly_12_bolditalic.h>
#include <builtinFonts/bookerly_12_italic.h>
#include <builtinFonts/bookerly_12_regular.h>
#include <builtinFonts/bookerly_16_bold.h>
#include <builtinFonts/bookerly_16_bolditalic.h>
#include <builtinFonts/bookerly_16_italic.h>
#include <builtinFonts/bookerly_16_regular.h>
#include <builtinFonts/bookerly_18_bold.h>
#include <builtinFonts/bookerly_18_bolditalic.h>
#include <builtinFonts/bookerly_18_italic.h>
#include <builtinFonts/bookerly_18_regular.h>
#include <builtinFonts/notosans_12_bold.h>
#include <builtinFonts/notosans_12_bolditalic.h>
#include <builtinFonts/notosans_12_italic.h>
#include <builtinFonts/notosans_12_regular.h>
#include <builtinFonts/notosans_14_bold.h>
#include <builtinFonts/notosans_14_bolditalic.h>
#include <builtinFonts/notosans_14_italic.h>
#include <builtinFonts/notosans_14_regular.h>
#include <builtinFonts/notosans_16_bold.h>
#include <builtinFonts/notosans_16_bolditalic.h>
#include <builtinFonts/notosans_16_italic.h>
#include <builtinFonts/notosans_16_regular.h>
#include <builtinFonts/notosans_18_bold.h>
#include <builtinFonts/notosans_18_bolditalic.h>
#include <builtinFonts/notosans_18_italic.h>
#include <builtinFonts/notosans_18_regular.h>
#include <builtinFonts/opendyslexic_10_bold.h>
#include <builtinFonts/opendyslexic_10_bolditalic.h>
#include <builtinFonts/opendyslexic_10_italic.h>
#include <builtinFonts/opendyslexic_10_regular.h>
#include <builtinFonts/opendyslexic_12_bold.h>
#include <builtinFonts/opendyslexic_12_bolditalic.h>
#include <builtinFonts/opendyslexic_12_italic.h>
#include <builtinFonts/opendyslexic_12_regular.h>
#include <builtinFonts/opendyslexic_14_bold.h>
#include <builtinFonts/opendyslexic_14_bolditalic.h>
#include <builtinFonts/opendyslexic_14_italic.h>
#include <builtinFonts/opendyslexic_14_regular.h>
#include <builtinFonts/opendyslexic_8_bold.h>
#include <builtinFonts/opendyslexic_8_bolditalic.h>
#include <builtinFonts/opendyslexic_8_italic.h>
#include <builtinFonts/opendyslexic_8_regular.h>

#elif defined(INCLUDE_EXTRA_READER_FONTS)
#include <builtinFonts/notosans_14_bold.h>
#include <builtinFonts/notosans_14_bolditalic.h>
#include <builtinFonts/notosans_14_italic.h>
#include <builtinFonts/notosans_14_regular.h>
#include <builtinFonts/opendyslexic_14_bold.h>
#include <builtinFonts/opendyslexic_14_bolditalic.h>
#include <builtinFonts/opendyslexic_14_italic.h>
#include <builtinFonts/opendyslexic_14_regular.h>

#endif  // OMIT_FONTS / INCLUDE_EXTRA_READER_FONTS
