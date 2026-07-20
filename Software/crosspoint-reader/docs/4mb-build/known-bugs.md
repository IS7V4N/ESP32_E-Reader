# 4MB Build — Ismert bugok

## BUG-001 — Webes fájlátvitel — File Manager tab 403 hiba

**Érintett build:** 4mb
**Első észlelés:** 2026-06-11
**Súlyosság:** Közepes — fájlkezelő tab nem elérhető
**Állapot:** Megoldva (2026-07-20)

### Tünet
A webes fájlátvitel felületen a három tab közül a File Manager tab 403 hibát dob (böngészőben "Not Found" szövegként jelenik meg a `/files` oldalon).

### Gyökérok
A `HomePage.html` mindig mutatja a "File Manager" tabot, de `CROSSPOINT_NO_WEBUI=1` esetén az `/files` route nincs regisztrálva. A kérést a WebDAV handler kapja el (minden GET-re `canHandle()` true), ami 404-et ad vissza, mert `/files` nem létezik az SD kártyán.

### javítási kísérletek (2026-07-18)
- `CrossPointWebServer::handleStatus()`: `hasFileManager` mező hozzáadva a JSON válaszhoz (`true`/`false` a compile-time flag alapján)
- `HomePage.html`: File Manager link alapból rejtett (`display:none`), JS csak akkor mutatja, ha `data.hasFileManager === true`
- Érintett fájlok: `src/network/CrossPointWebServer.cpp:349`, `src/network/html/HomePage.html`

### Végleges javítás (2026-07-20)
A `CROSSPOINT_NO_WEBUI=1` flag eltávolítva a `[env:4mb]`-ből (`platformio.ini`). A flag bevezetésekor becsült 425KB-os megtakarítás téves volt: a FilesPage HTML és a jszip gzip-elve van beágyazva a firmware-be, a tényleges méretnövekedés csak ~84KB (mért: 3 099 795 → 3 183 937 bájt). A 3 125 KB-os (0x320000) factory partícióban így is ~90KB szabad hely marad. A `hasFileManager` flag ezután automatikusan `true`-t jelent, a File Manager tab a `HomePage.html`-en normálisan megjelenik, a `/files` route pedig ténylegesen ki van szolgálva — a fenti workaroundok (rejtett tab) érdemben feleslegessé váltak, de ártalmatlanok, nem lettek visszavonva.
- Érintett fájl: `platformio.ini` (`[env:4mb]` build_flags, a `-D CROSSPOINT_NO_WEBUI=1` sor törölve)

## BUG-002 — Display szöveg elhalványulás / alacsony kontraszt

**Érintett build:** 4mb
**Első észlelés:** 2026-06-10
**Súlyosság:** Közepes — olvashatóságot rontja, de nem crash
**Állapot:**

### Tünet
Könyv megnyitásakor (és lapozáskor) a szöveg halványabban jelenik meg anti-aliasing bekapcsolt állapotban.

### Gyökérok
A `displayGrayBuffer()` (grayscale AA render) után a RED RAM-ot felülírja a `cleanupGrayscaleBuffers()` az MSB grayscale adatokról az eredeti BW tartalomra. Ezután amikor `displayBuffer()` meghívja `grayscaleRevert()`-et (a következő lapozásnál), a revert LUT rossz adatokat lát:
- BW RAM = LSB (grayscale) ✓
- RED RAM = eredeti BW tartalom ✗ (felülírta a `cleanupGrayscaleBuffers`)

A revert LUT a `00` esetet (`BW=0, RED=0`) "nincs változás"-nak értelmezi, tehát a fehér háttérpixelek (mindkettő 0) nem kapnak feszültséget → szürke állapotban maradnak → halványság.

### javítási kísérletek (2026-07-18)
`pagesUntilFullRefresh = 1` beállítása a grayscale display után:
- A következő lapozásnál `displayWithRefreshCycle()` HALF_REFRESH-t végez
- A HALF_REFRESH CTRL1_BYPASS_RED: nem differenciális, minden pixelt a BW RAM alapján B/F-re hajt, függetlenül az előző állapottól
- Érintett fájlok: `EpubReaderActivity.cpp:782`, `TxtReaderActivity.cpp:382`

### Korábbi javítási kísérletek (visszavonva)
- CTRL1_BYPASS_RED / 0xFC / CMD_AUTO_WRITE_RED_RAM módosítások → üres képernyő
- HALF_REFRESH → FULL_REFRESH csere a ReaderUtils.h-ban → vízszintes vonalak

Részletek: `docs/4mb-build/changes/2026-06-10_fix_half-refresh-temperature.md`
