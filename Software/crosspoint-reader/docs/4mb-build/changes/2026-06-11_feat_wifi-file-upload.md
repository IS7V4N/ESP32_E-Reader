# feat: WiFi fájlfeltöltés (WebDAV) — 4mb build

**Dátum:** 2026-06-11  
**Típus:** feat  
**Érintett fájlok:**
- `platformio.ini`
- `src/network/CrossPointWebServer.cpp`
- `src/network/CrossPointWebServer.h`
- `src/activities/settings/SettingsActivity.cpp`
- `src/activities/home/HomeActivity.cpp`
- `src/activities/home/HomeActivity.h`

## Motiváció

A 4mb build korábban teljesen ki volt zárva a WiFi funkció (`CROSSPOINT_NO_WIFI=1`).
Flash kihasználtság ~70% volt a font-slim csomag után, ~885KB szabad.
A fő blokkoló a webes fájlböngésző volt: FilesPage HTML (251KB) + jszip.min.js (174KB) = 425KB.

Cél: WebDAV fájlfeltöltés visszarakása, miközben a nehéz webes UI ki van zárva.

## Megvalósítás

### Új flag architektúra

Egy átfogó `CROSSPOINT_NO_WIFI` helyett granulált flagek:

| Flag | Mit zár ki | 4mb build |
|------|-----------|-----------|
| `CROSSPOINT_NO_WIFI` | WiFi hardver + WifiSelectionActivity UI | **NINCS** (WiFi engedélyezve) |
| `CROSSPOINT_NO_WEBUI` | FilesPage HTML + jszip + HTTP fájlböngésző routeok | **BE** |
| `CROSSPOINT_NO_KOREADER` | KOReader szinkron, OPDS böngésző | **BE** |
| `CROSSPOINT_NO_OTA` | OTA frissítő (nincs OTA partíció 4MB-on) | **BE** |

### Elérhető WiFi funkciók 4mb buildben

- **WiFi hálózat kiválasztás** (WifiSelectionActivity) — Beállítások menüben
- **WebDAV fájlfeltöltés** — WebDAVHandler (változatlan)
- **Webes beállítások oldal** — `/settings` HTTP végpont
- **Státusz API** — `/status` HTTP végpont
- **WebSocket feltöltés** — WsUploadStatus alapú, valós idejű progressz

### Kizárt funkciók 4mb buildben

- Webes fájlböngésző UI (`/files`, `/api/files`, `/download`, stb.)
- jszip.min.js fájl
- KOReader szinkron (`KOReaderSettingsActivity`, `KOReaderSyncActivity`)
- OPDS böngésző (`CalibreSettingsActivity` UI menüpont — maga az osztály lefordul)
- OTA frissítés (`OtaUpdateActivity`, `OtaUpdater`)

## Módosítások

### `src/network/CrossPointWebServer.cpp` és `.h`

`#ifndef CROSSPOINT_NO_WEBUI` guard-ok a következőkre:
- FilesPageHtml és jszip_minJs include-ok
- HTTP route regisztráció: `/files`, `/js/jszip.min.js`, `/api/files`, `/download`,
  `/upload`, `/mkdir`, `/rename`, `/move`, `/delete`
- UploadState struct (multipart POST feltöltéshez)
- `handleJszip()`, `handleFileList()`, `handleDownload()`, `handleUpload()`,
  `handleUploadPost()`, `handleCreateFolder()`, `handleRename()`, `handleMove()`,
  `handleDelete()`, `scanFiles()`, `isEpubFile()`, `formatFileSize()`

Megjegyzés: `handleSettingsPage()`, `handleGetSettings()`, `handlePostSettings()`,
WebDAV handler regisztráció, és WebSocket feltöltés **NEM** guarded — mindig elérhető.

### `src/activities/settings/SettingsActivity.cpp`

Include-ok szétválasztva:
```cpp
#ifndef CROSSPOINT_NO_WIFI
#include "activities/network/WifiSelectionActivity.h"
#endif
#ifndef CROSSPOINT_NO_KOREADER
#include "KOReaderSettingsActivity.h"
#endif
#ifndef CROSSPOINT_NO_OTA
#include "OtaUpdateActivity.h"
#endif
```

Menüpontok granuláltan:
- `WIFI_NETWORKS` → `CROSSPOINT_NO_WIFI` guard
- `KOREADER_SYNC`, `OPDS_BROWSER` → `CROSSPOINT_NO_KOREADER` guard
- `CHECK_UPDATES` → `CROSSPOINT_NO_OTA` guard

Action handler case-ek szintén granuláltan szétválasztva.

### `src/activities/home/HomeActivity.cpp` és `.h`

OPDS böngészőre vonatkozó `CROSSPOINT_NO_WIFI` → `CROSSPOINT_NO_KOREADER` cserék:
- `hasOpdsUrl` tagváltozó deklaráció
- `hasOpdsUrl` inicializálás `onEnter()`-ben
- `getMenuItemCount()` és `render()` OPDS feltétel
- `onOpdsBrowserOpen()` deklaráció és implementáció

`onFileTransferOpen()` WiFi-guard eltávolítva — mindig `activityManager.goToFileTransfer()`.

### `platformio.ini` — `[env:4mb]`

```ini
-D CROSSPOINT_NO_WEBUI=1
-D CROSSPOINT_NO_KOREADER=1
-D CROSSPOINT_NO_OTA=1
```

`CROSSPOINT_NO_WIFI` **nincs** definiálva → WiFi hardver és WifiSelectionActivity aktív.

`build_src_filter` includeok hozzáadva (előző lépésből):
```
+<network/> és +<activities/network/>
```

Kizárva marad:
```
-<network/OtaUpdater.cpp>
-<activities/reader/KOReaderSyncActivity.cpp>
-<activities/settings/KOReaderAuthActivity.cpp>
-<activities/settings/KOReaderSettingsActivity.cpp>
-<activities/settings/OtaUpdateActivity.cpp>
```

## Várható flash hatás

| Elem | Méret |
|------|-------|
| FilesPage HTML | ~251KB |
| jszip.min.js | ~174KB |
| WebSocketsServer könyvtár | ~15KB |
| **Nettó változás** | **~−440KB** (file browser nélkül) |

## Ellenőrzés

`pio run -e 4mb` — fordítás hibamentes kell legyen.

Eszközön:
1. WiFi hálózat kiválasztás megjelenik a Beállítások → Rendszer menüben
2. KOReader Sync és OPDS Browser **nem jelenik meg**
3. Frissítés keresése **nem jelenik meg**
4. Fájl átvitel gomb a főmenüben megnyitja a WiFi fájlfeltöltés felületet
5. WebDAV klienssel (pl. WinSCP, Cyberduck) fájlok feltölthetők
