# Frontlight

The frontlight is driven by a PWM signal generated via the ESP32-C3's LEDC peripheral. All logic lives in `lib/hal/HalFrontlight.h` and `lib/hal/HalFrontlight.cpp`.

---

## Hardware

| Property        | Value                                     |
|-----------------|-------------------------------------------|
| GPIO pin        | 20                                        |
| PWM frequency   | 10 kHz (LED driver range: 5–100 kHz)      |
| PWM resolution  | 8-bit (duty range 0–255)                  |
| Driver polarity | Active-high (higher duty = brighter)      |

GPIO 20 is shared with the USB presence detection line. Because the frontlight repurposes this pin for PWM output, USB connection detection is permanently disabled (`HalGPIO::isUsbConnected()` always returns `false`).

---

## PWM Brightness Mapping

The LED driver is **active-high**: duty 0 = LED off, higher duty = brighter. No signal inversion is applied in software.

The observed working range of the hardware is approximately `[0, 60]` duty counts (8-bit LEDC). Values above ~60 saturate to the same perceived brightness.

The mapping spreads the full 1–100 % user range linearly across `[PWM_DUTY_MIN, PWM_DUTY_MAX]`:

```
if percent == 0:
    duty = 0                                        // constant LOW -> LED off
else:
    duty = PWM_DUTY_MIN
          + (percent - 1) * (PWM_DUTY_MAX - PWM_DUTY_MIN) / 99
```

| Constant        | Value | Purpose                                              |
|-----------------|-------|------------------------------------------------------|
| `PWM_DUTY_MIN`  | 1     | Duty for 1 % UI value (dim but stable)               |
| `PWM_DUTY_MAX`  | 60    | Duty for 100 % UI value; LED saturates above this    |

Both constants are in `lib/hal/HalFrontlight.h` and are the only values that need tuning if the hardware response changes.

| User % | Duty written | Notes                      |
|--------|--------------|----------------------------|
| 0      | 0            | Off                        |
| 1      | 1            | Minimum visible brightness |
| 10     | 6            |                            |
| 50     | 30           |                            |
| 90     | 54           |                            |
| 100    | 60           | Maximum brightness         |

---

## API

All methods are `static`; no instance is required.

### `HalFrontlight::begin(uint8_t brightnessPercent)`

Attaches the LEDC channel to `FRONTLIGHT_PIN` and immediately applies `brightnessPercent`. Called once during firmware boot (`src/main.cpp`) after settings are loaded.

```cpp
HalFrontlight::begin(SETTINGS.frontlightBrightness);
```

Logs `LOG_ERR` and returns early if `ledcAttach()` fails.

### `HalFrontlight::setBrightness(uint8_t percent)`

Updates the duty cycle at runtime. Values above 100 are clamped to 100.

```cpp
HalFrontlight::setBrightness(50);  // duty = 30/255
```

### `HalFrontlight::off()`

Sets duty to 0 immediately. Called in `enterDeepSleep()` before the device enters deep sleep.

```cpp
HalFrontlight::off();
```

---

## Lifecycle Integration

```
Boot
 └─ setup()
     ├─ SETTINGS.loadFromFile()
     └─ HalFrontlight::begin(SETTINGS.frontlightBrightness)   ← apply saved brightness

Main loop
 └─ loop()
     ├─ if (SETTINGS.frontlightBrightness changed)
     │   └─ HalFrontlight::setBrightness(newValue)            ← settings panel change
     └─ double-press detection
         └─ HalFrontlight::setBrightness(0 or savedValue)     ← power button toggle

Deep sleep
 └─ enterDeepSleep()
     └─ HalFrontlight::off()                                  ← turn off before sleep
```

The main loop polls `SETTINGS.frontlightBrightness` against a cached `lastFrontlight` value. There is no interrupt or callback — changes take effect within one loop iteration.

---

## Power Button Double-Press Toggle

A double-press of the power button (two releases within 300 ms) toggles the frontlight between off (0 %) and the currently configured `SETTINGS.frontlightBrightness`.

The toggle state (`frontlightSuppressed`) is tracked as a `static bool` in `loop()`. An explicit change via the settings panel clears the suppressed state and applies the new value directly.

**Lookahead buffering** prevents accidental page turns in `PAGE_TURN` mode:
- The first release is consumed (hidden from activities) and parked for 300 ms.
- If a second release arrives within the window, the frontlight toggles and neither release reaches any activity.
- If the window expires with no second press, the first release is replayed via `gpio.forceRelease()` so the activity processes it normally (with ≤300 ms latency, imperceptible on e-ink).

See `docs/4mb-build/changes/2026-07-18_feat_frontlight-double-press.md` for implementation details.

**Limitations:**
- Not functional when `shortPwrBtn == SLEEP` (any press immediately triggers deep sleep).
- Toggle state does not survive deep sleep.

---

## User Setting

The brightness is persisted as `CrossPointSettings::frontlightBrightness` (`uint8_t`, default `0`).

| Property      | Value                                      |
|---------------|--------------------------------------------|
| Setting key   | `frontlightBrightness`                     |
| Range         | 0–100 (step 10)                            |
| Default       | 0 (off)                                    |
| UI label      | `STR_FRONTLIGHT`                           |
| Settings page | Display (`STR_CAT_DISPLAY`)                |
| Defined in    | `src/SettingsList.h`, `src/CrossPointSettings.h` |

---

## Logging

All log messages use the `"FLT"` module tag.

| Condition              | Level     | Message example                              |
|------------------------|-----------|----------------------------------------------|
| LEDC attach fails      | `LOG_ERR` | `LEDC attach failed on GPIO 20`              |
| PWM initialised        | `LOG_DBG` | `Frontlight PWM initialised on GPIO 20`      |
| Brightness changed     | `LOG_DBG` | `Brightness 50% -> duty 30/255`              |
| Double-press off       | `LOG_DBG` | `Double-press: frontlight off`               |
| Double-press restore   | `LOG_DBG` | `Double-press: frontlight restored`          |

---

## Tuning

Edit `PWM_DUTY_MIN` and `PWM_DUTY_MAX` in `lib/hal/HalFrontlight.h`:

```cpp
static constexpr uint8_t PWM_DUTY_MIN = 1;   // raise if 1% flickers
static constexpr uint8_t PWM_DUTY_MAX = 60;  // raise if 100% seems too dim
```

No other files need to change. The user-facing 0–100 % scale re-maps automatically.
