# fix: HALF_REFRESH belső hőmérséklet-érzékelő használata

**Dátum:** 2026-06-10  
**Típus:** fix  
**Érintett fájl:** `open-x4-sdk/libs/display/EInkDisplay/src/EInkDisplay.cpp:617-621`

## Probléma

A HALF_REFRESH mód manuálisan `0x5A` (= 90°C) értéket írt a hőmérséklet-regiszterbe
(`CMD_WRITE_TEMP`, 0x1A), és nem állította be a `TEMP_LOAD` bitet (0x20) a display
update control parancsban.

Ennek következménye:
- A vezérlő a "forró klíma" gyors OTP hullámformát alkalmazta szobahőmérsékleten
- Ez a hullámforma ~480ms alatt fut le, de nem hajtja teljesen feketébe/fehérbe a pixeleket
- Az ezt követő szürkeárnyalatos LUT rosszul meghatározott állapotból indul
- **Tünet:** a szöveg szürkére halványul lapozás / könyv megnyitásakor

## Javítás

```cpp
// Előtte:
} else if (mode == HALF_REFRESH) {
    // Write high temp to the register for a faster refresh
    sendCommand(CMD_WRITE_TEMP);
    sendData(0x5A);
    displayMode |= 0xDC;  // TEMP_LOAD nincs beállítva!

// Utána:
} else if (mode == HALF_REFRESH) {
    displayMode |= 0xFC;  // 0xDC | 0x20 = TEMP_LOAD hozzáadva
```

`0xFC = 0x80 (CLOCK_ON) | 0x40 (ANALOG_ON) | 0x20 (TEMP_LOAD) | 0x10 (LUT_LOAD) | 0x08 (MODE_SELECT) | 0x04 (DISPLAY_START)`

## Miért működik

Az SSD1677 beépített hőmérséklet-érzékelővel rendelkezik, ami az inicializáláskor
be van kapcsolva (`CMD_TEMP_SENSOR_CONTROL` / `0x80`, `EInkDisplay.cpp:236`).

A `TEMP_LOAD` bit engedélyezésével a vezérlő a tényleges hőmérsékletnek megfelelő
OTP hullámformát választja. Szobahőmérsékleten (~25°C) ez lassabb (~750-1000ms),
de teljesen fekete/fehérbe hajtja a pixeleket — a szürkeárnyalatos LUT ezután
helyes kiindulóállapotból dolgozik.

Külső hőmérséklet-érzékelőre **nincs szükség**: a `TEMP_SENSOR_INTERNAL` (0x80)
már be van állítva az inicializálásban, és a `TEMP_LOAD` bit azt a belső érzékelőt
olvassa.

## Ellenőrzés

Hardware teszten: lapozás utáni szöveg és könyv megnyitásakor megjelenő szöveg
feketén marad, nem halványul szürkére. A HALF_REFRESH lassabb lesz (~750-1000ms
vs. ~480ms korábban), ez normális és elvárható szobahőmérsékleten.
