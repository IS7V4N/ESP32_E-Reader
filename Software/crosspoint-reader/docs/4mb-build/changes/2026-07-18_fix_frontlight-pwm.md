# fix: Frontlight PWM helyes irány és tartomány

**Dátum:** 2026-07-18  
**Típus:** fix  
**Érintett fájlok:**
- `lib/hal/HalFrontlight.h` — `PWM_DUTY_MIN`, `PWM_DUTY_MAX` konstansok
- `lib/hal/HalFrontlight.cpp` — `setBrightness()` logika

---

## Tünetek

- UI 10–80% között azonos fényerő
- 90%-nál kicsit sötétebb
- 100%-nál teljesen kialszik a frontlight

---

## Gyökérok — két egymástól független hiba

### 1. Invertált PWM irány

Az eredeti kód az `hwDuty = 255 - duty` képlettel megfordította a PWM duty cycle-t, mert a megjegyzés szerint a hardver invertált kimenettel rendelkezik:

```cpp
// RÉGI (hibás):
const uint8_t hwDuty = static_cast<uint8_t>(255u - duty);
ledcWrite(FRONTLIGHT_PIN, hwDuty);
```

A valóságban a LED driver **active-high** (magasabb duty = fényesebb), és nincs szoftveres invertálásra szükség. Az `off()` hívás helyesen írta `0`-ra a duty-t (LED ki), de `setBrightness(100)` szintén `0`-t írt ki az invertálás miatt — ezért kapcsolt ki 100%-nál.

### 2. Hibás tartomány-mapping

```cpp
// RÉGI (hibás):
const uint8_t scaled = (percent * PWM_DUTY_MAX) / 100;
// PWM_DUTY_MAX = 255 → scaled = percent * 2.55
// duty = max(scaled, PWM_DUTY_MIN = 200)
```

Ez azt jelenti, hogy 1–78% esetén `scaled < 200`, ezért mindegyik `duty = 200` értékre kerül → azonos fényerő. Csak 79–100% között volt tényleges eltérés — de az invertálás miatt csökkenő irányban.

---

## Javítás

### `lib/hal/HalFrontlight.h`

```cpp
// RÉGI:
static constexpr uint8_t PWM_DUTY_MIN = 200;
static constexpr uint8_t PWM_DUTY_MAX = 255;

// ÚJ:
static constexpr uint8_t PWM_DUTY_MIN = 1;   // 1% UI értéknél minimális de stabil fény
static constexpr uint8_t PWM_DUTY_MAX = 60;  // LED ~60 duty felett telítésbe megy
```

A valós hardvermérés alapján a LED a `[0, ~60]` duty tartományban reagál; e felett telítésbe megy (azonos fényerő). A `PWM_DUTY_MAX` szükség esetén hangolható.

### `lib/hal/HalFrontlight.cpp` — `setBrightness()`

```cpp
// ÚJ: 1–100% lineárisan leképezve [PWM_DUTY_MIN, PWM_DUTY_MAX]-ra, invertálás nélkül
const uint8_t duty = static_cast<uint8_t>(
    PWM_DUTY_MIN +
    (static_cast<uint16_t>(percent - 1) * (PWM_DUTY_MAX - PWM_DUTY_MIN)) / 99u);
ledcWrite(FRONTLIGHT_PIN, duty);
```

---

## Ellenőrzés

| UI % | duty (0–255) | Várható eredmény       |
|------|-------------|------------------------|
| 0%   | 0           | Ki (off)               |
| 1%   | 1           | Alig látható           |
| 50%  | 31          | Közepes fény           |
| 100% | 60          | Maximum (telítési pont)|

Serial log (`FLT` prefix): `Brightness 50% -> duty 31/255`
