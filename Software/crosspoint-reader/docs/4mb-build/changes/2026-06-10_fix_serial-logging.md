# fix: Serial logging feltétel nélkül indul

**Dátum:** 2026-06-10  
**Típus:** fix  
**Érintett fájl:** `src/main.cpp` (~239. sor)

## Módosítás

A `Serial.begin()` hívás korábban `gpio.isUsbConnected()` feltételhez volt kötve.
Ez a függvény (`lib/hal/HalGPIO.cpp:25-28`) mindig `false`-t ad vissza,
mert a GPIO20 pint az eredeti USB-észlelés céljára tervezték, de a 4MB build
hardveren ezt a pint a frontlight PWM-vezérlés foglalja el.

**Eredmény:** A serial monitor soha nem kapott adatot, a `debugging_monitor.py`
üres kimenetet mutatott.

## Javítás

Eltávolítottuk az `isUsbConnected()` feltételt. A Serial mindig elindul, és legfeljebb
3 másodpercig vár USB host csatlakozásra. Ha nincs host, a boot folytatódik blokkolás nélkül.

**FONTOS — pozícionálás:** A `Serial.begin()` blokkot a `verifyPowerButtonDuration()` UTÁN
kell elhelyezni (a wakeup-reason switch/case után). Ha előtte van, a 3 másodperces
USB-várakozás eltolódást okoz, és a gombnyomás-ellenőrzés mindig `abort`-ot ad vissza
(a gomb már elengedett, mire a kód ideér) → kétszeri gombnyomást igényel a bekapcsoláshoz.

```cpp
// Előtte:
if (gpio.isUsbConnected()) {
  Serial.begin(115200);
  ...
}

// Utána (helyes pozícióban, AFTER verifyPowerButtonDuration()):
Serial.begin(115200);
{
  unsigned long serialStart = millis();
  while (!Serial && (millis() - serialStart) < 3000) {
    delay(10);
  }
}
```

## Ok

A 4MB build hardveren a GPIO20 frontlight PWM-re van kötve, nem USB VBUS észlelésre.
Az eredeti kódban lévő `isUsbConnected()` hívás ezért sosem tér vissza `true`-val.
