# feat: Slim font szett — INCLUDE_EXTRA_READER_FONTS flag

**Dátum:** 2026-06-11  
**Típus:** feat  
**Érintett fájlok:**
- `lib/EpdFont/builtinFonts/all.h`
- `src/main.cpp`
- `src/CrossPointSettings.cpp`
- `platformio.ini`

## Motiváció

A 4mb build `OMIT_FONTS=1` flaggel csak Bookerly 14pt + UI fontokat tartalmazott.
Flash kihasználtság ~58% volt — elegendő hely maradt Noto Sans és OpenDyslexic
14pt hozzáadásához, hogy mindhárom fontcsalád elérhető legyen.

## Új build flag: `INCLUDE_EXTRA_READER_FONTS`

Csak `OMIT_FONTS=1` mellé értelmezett. Ha mindkettő definiálva van:

| Font | Stílusok |
|------|----------|
| Bookerly 14pt | regular, bold, italic, bolditalic *(korábban is bent volt)* |
| Noto Sans 14pt | regular, bold, italic, bolditalic *(új)* |
| OpenDyslexic 14pt | regular, bold, italic, bolditalic *(új)* |
| UI fontok (Ubuntu 10/12, Noto Sans 8) | *(változatlan)* |

## Módosítások

### `lib/EpdFont/builtinFonts/all.h`
- `#ifndef OMIT_FONTS` → `#if !defined(OMIT_FONTS)` (szemantikailag azonos, konzisztensebb)
- Új `#elif defined(INCLUDE_EXTRA_READER_FONTS)` blokk: Noto Sans 14 és OpenDyslexic 14
  header fájlok (8 db, 4-4 stílus)

### `src/main.cpp`
- Globális font objektumok: `#elif defined(INCLUDE_EXTRA_READER_FONTS)` blokkban
  `notosans14*` és `opendyslexic14*` EpdFont + EpdFontFamily objektumok
- `setupDisplayAndFonts()`: `#elif` blokkban `renderer.insertFont()` hívások a két új
  fontcsaládhoz

### `src/CrossPointSettings.cpp` — `getReaderFontId()`
Slim módban a font méret beállítás figyelmen kívül marad — mindig a 14pt variáns
kerül visszaadásra az adott fontcsaládhoz:

```cpp
#if defined(OMIT_FONTS)
  #if defined(INCLUDE_EXTRA_READER_FONTS)
    switch (fontFamily) {
      case NOTOSANS:     return NOTOSANS_14_FONT_ID;
      case OPENDYSLEXIC: return OPENDYSLEXIC_14_FONT_ID;
      default:           return BOOKERLY_14_FONT_ID;
    }
  #else
    return BOOKERLY_14_FONT_ID;
  #endif
#else
  // ... teljes méret-switch változatlan
#endif
```

### `platformio.ini` — `[env:4mb]`
```ini
-D OMIT_FONTS=1
-D INCLUDE_EXTRA_READER_FONTS=1
```

## Flag kombinációk összefoglalója

| Flag kombináció | Elérhető fontok |
|-----------------|-----------------|
| Egyik sem | Teljes csomag: Bookerly 12/14/16/18, Noto Sans 12/14/16/18, OpenDyslexic 8/10/12/14 |
| `OMIT_FONTS=1` | Csak Bookerly 14 + UI |
| `OMIT_FONTS=1` + `INCLUDE_EXTRA_READER_FONTS=1` | Bookerly 14 + Noto Sans 14 + OpenDyslexic 14 + UI |

## Megjegyzések

- `INCLUDE_EXTRA_READER_FONTS` önmagában (OMIT_FONTS nélkül) nem csinál semmit —
  a teljes fontcsomag úgyis mindent tartalmaz, az `#elif` ág nem fut le
- Font méret beállítás (`fontSize`) slim módban nem érvényesül — a 14pt fix
  (elfogadható korlát 4mb buildben)
- Cache érvénytelenítés nem szükséges — font ID-k nem változtak, csak elérhetővé váltak
