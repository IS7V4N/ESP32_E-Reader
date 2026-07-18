# 4MB Build — Ismert bugok

## BUG-001 — Webes fájlátvitel — File Manager tab 403 hiba

**Érintett build:** 4mb  
**Első észlelés:** 2026-06-11  
**Súlyosság:** Közepes — fájlkezelő tab nem elérhető, de feltöltés és egyéb funkciók működhetnek
**Állapot:** ✅ MEGOLDVA (2026-07-18)

### Tünet
A webes fájlátvitel felületen a három tab közül a File Manager tab 403/404 hibát dob. A többi tab érintetlen.

### Gyökérok
A `HomePage.html` mindig mutatja a "File Manager" tabot, de `CROSSPOINT_NO_WEBUI=1` esetén az `/files` route nincs regisztrálva. A kérést a WebDAV handler kapja el (minden GET-re `canHandle()` true), ami 404-et ad vissza, mert `/files` nem létezik az SD kártyán.

### Megoldás (2026-07-18)
- `CrossPointWebServer::handleStatus()`: `hasFileManager` mező hozzáadva a JSON válaszhoz (`true`/`false` a compile-time flag alapján)
- `HomePage.html`: File Manager link alapból rejtett (`display:none`), JS csak akkor mutatja, ha `data.hasFileManager === true`
- Érintett fájlok: `src/network/CrossPointWebServer.cpp:349`, `src/network/html/HomePage.html`

## BUG-002 — Display szöveg elhalványulás / alacsony kontraszt

**Érintett build:** 4mb
**Első észlelés:** 2026-06-10
**Súlyosság:** Közepes — olvashatóságot rontja, de nem crash
**Állapot:** ✅ MEGOLDVA (2026-07-18)

### Tünet
Könyv megnyitásakor (és lapozáskor) a szöveg halványabban jelenik meg anti-aliasing bekapcsolt állapotban.

### Gyökérok
A `displayGrayBuffer()` (grayscale AA render) után a RED RAM-ot felülírja a `cleanupGrayscaleBuffers()` az MSB grayscale adatokról az eredeti BW tartalomra. Ezután amikor `displayBuffer()` meghívja `grayscaleRevert()`-et (a következő lapozásnál), a revert LUT rossz adatokat lát:
- BW RAM = LSB (grayscale) ✓
- RED RAM = eredeti BW tartalom ✗ (felülírta a `cleanupGrayscaleBuffers`)

A revert LUT a `00` esetet (`BW=0, RED=0`) "nincs változás"-nak értelmezi, tehát a fehér háttérpixelek (mindkettő 0) nem kapnak feszültséget → szürke állapotban maradnak → halványság.

### Megoldás (2026-07-18)
`pagesUntilFullRefresh = 1` beállítása a grayscale display után:
- A következő lapozásnál `displayWithRefreshCycle()` HALF_REFRESH-t végez
- A HALF_REFRESH CTRL1_BYPASS_RED: nem differenciális, minden pixelt a BW RAM alapján B/F-re hajt, függetlenül az előző állapottól
- Érintett fájlok: `EpubReaderActivity.cpp:782`, `TxtReaderActivity.cpp:382`

### Korábbi javítási kísérletek (visszavonva — NE próbáld újra)
- CTRL1_BYPASS_RED / 0xFC / CMD_AUTO_WRITE_RED_RAM módosítások → üres képernyő
- HALF_REFRESH → FULL_REFRESH csere a ReaderUtils.h-ban → vízszintes vonalak

Részletek: `docs/4mb-build/changes/2026-06-10_fix_half-refresh-temperature.md`
