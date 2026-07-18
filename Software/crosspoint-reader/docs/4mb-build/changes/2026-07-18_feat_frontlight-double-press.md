# feat: Frontlight toggle dupla bekapcsoló gombbal

**Dátum:** 2026-07-18  
**Típus:** feat  
**Érintett fájlok:**
- `open-x4-sdk/libs/hardware/InputManager/include/InputManager.h`
- `open-x4-sdk/libs/hardware/InputManager/src/InputManager.cpp`
- `lib/hal/HalGPIO.h`
- `lib/hal/HalGPIO.cpp`
- `src/main.cpp`

---

## Funkció

A bekapcsoló gomb kétszeri gyors megnyomása (dupla press, max 300 ms-on belül) kapcsol a frontlight **0%** és a **beállításokban tárolt értéke** között.

Ha a beállításokban a felhasználó megváltoztatja a fényerőt, a toggle állapot törlődik és az új érték lép érvénybe.

---

## Implementáció

### InputManager — consume/force API (`open-x4-sdk`)

Két új metódus a bitmaszk-szintű eseménykezeléshez:

```cpp
// InputManager.h
void consumeRelease(uint8_t buttonIndex);  // törli a released bitet
void forceRelease(uint8_t buttonIndex);    // beállítja a released bitet
```

```cpp
// InputManager.cpp
void InputManager::consumeRelease(uint8_t idx) { releasedEvents &= ~(1u << idx); }
void InputManager::forceRelease(uint8_t idx)   { releasedEvents |=  (1u << idx); }
```

Ezek a metódusok nem hívnak `update()`-et, csak az aktuális tick `releasedEvents` bitmaszáját módosítják. A következő `update()` hívásig maradnak érvényben.

### HalGPIO wrapper

```cpp
// HalGPIO.h / HalGPIO.cpp
void consumeRelease(uint8_t buttonIndex);
void forceRelease(uint8_t buttonIndex);
```

### Lookahead puffer — `src/main.cpp`

A klasszikus dupla-press detektálás problémája PAGE_TURN módban: az első release már lapot fordít, mielőtt tudnánk, hogy dupla press következik.

**Megoldás: lookahead delay**  
Az első release-t elrejtjük az activity elől (consume), és 300 ms-ig parkoljuk. Ha lejár az ablak anélkül, hogy második nyomás jönne, visszajátsszuk (force). Ha jön a második, a frontlight togglel — egyik release sem ér el az activity-hoz.

```
Egyszeres nyomás (pl. lapfordítás):
  1. release → consumeRelease, pendingPowerReleaseMs = now
  2. 300 ms telik el, nincs második nyomás
  3. forceRelease → activity látja → lapfordítás (300 ms késéssel)

Dupla nyomás:
  1. 1. release → consumeRelease, pendingPowerReleaseMs = now
  2. 2. release < 300 ms-en belül → double-press detektálva
  3. frontlight toggle
  4. Sem az első, sem a második release nem ér el az activity-hoz → nincs lapfordítás
```

**300 ms késés egyszeres nyomásnál** e-ink displayen nem észrevehető, mivel maga a kijelző frissítése 1–2 másodpercet vesz igénybe.

### Kódrészlet (`src/main.cpp`)

```cpp
static constexpr unsigned long DOUBLE_PRESS_MS = 300;
static unsigned long pendingPowerReleaseMs = 0;

if (gpio.wasReleased(HalGPIO::BTN_POWER)) {
  const unsigned long now = millis();
  if (pendingPowerReleaseMs != 0 && now - pendingPowerReleaseMs <= DOUBLE_PRESS_MS) {
    frontlightSuppressed = !frontlightSuppressed;
    HalFrontlight::setBrightness(frontlightSuppressed ? 0 : SETTINGS.frontlightBrightness);
    pendingPowerReleaseMs = 0;
  } else {
    gpio.consumeRelease(HalGPIO::BTN_POWER);
    pendingPowerReleaseMs = now;
  }
}

if (pendingPowerReleaseMs != 0 && millis() - pendingPowerReleaseMs > DOUBLE_PRESS_MS) {
  gpio.forceRelease(HalGPIO::BTN_POWER);
  pendingPowerReleaseMs = 0;
}
```

---

## Ismert korlátok

- **SLEEP mód** (`shortPwrBtn == SLEEP`, `getPowerButtonDuration() = 10 ms`): bármilyen gombnyomás azonnal alvó módba viszi az eszközt, ezért dupla nyomás ebben a módban nem lehetséges.
- **Egyszeres lapfordítás késése**: PAGE_TURN módban a lapfordítás 300 ms-t késik (a lookahead ablak miatt). Ez e-ink displayen nem érzékelhető.
- A toggle állapota (`frontlightSuppressed`) **nem marad meg** mély alvás után.

---

## Ellenőrzés

Serial log (`FLT` prefix):
- `Double-press: frontlight off` — toggle ki
- `Double-press: frontlight restored` — toggle vissza
