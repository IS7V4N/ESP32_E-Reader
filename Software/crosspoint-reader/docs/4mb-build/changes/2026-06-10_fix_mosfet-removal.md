# fix: Battery latch MOSFET kód eltávolítva

**Dátum:** 2026-06-10  
**Típus:** fix  
**Érintett fájl:** `lib/hal/HalPowerManager.cpp` — `startDeepSleep()` metódus

## Módosítás

Eltávolítottuk a GPIO13 battery latch MOSFET vezérlő kódot a `startDeepSleep()` metódusból.

```cpp
// Eltávolított kódblokk:
constexpr gpio_num_t GPIO_SPIWP = GPIO_NUM_13;
gpio_set_direction(GPIO_SPIWP, GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_SPIWP, 0);
esp_sleep_config_gpio_isolate();
gpio_deep_sleep_hold_en();
gpio_hold_en(GPIO_SPIWP);
```

## Ok

A battery latch MOSFET (GPIO13) nincs beépítve a 4MB build hardverre. Ha a kód
mégis futott volna, a GPIO13-at (ami az ESP32-C3 SPI WP / Write Protect lába) OUTPUT
módba és alacsony szintre húzta volna — felesleges, és potenciálisan zavarhatta
az SPI flash kommunikációt.

## Hatás

- A deep sleep továbbra is működik az `esp_deep_sleep_start()` hívással
- A power button wakeup (`esp_deep_sleep_enable_gpio_wakeup`) érintetlen maradt
- **Pozitív mellékhatás:** Az `RTC_NOINIT_ATTR` log ring buffer megőrzi tartalmát
  sleep ciklusok között (a MOSFET az RTC RAM-ot is kikapcsolta volna)
