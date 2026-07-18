# ESP32 E-Reader

A custom-built e-ink reading device based on the ESP32-C3, designed for everyday use. Built around a 4.2" e-ink display with a frontlight for reading in the dark enviroment, running a modified version of the [Crosspoint](https://github.com/crosspoint-reader/crosspoint-reader) open-source firmware adapted for custom hardware. The goal was to build a capable, low-cost alternative to commercial e-readers with full control over both hardware and software.

> **Note:** This is a fork of [Crosspoint Reader](https://github.com/crosspoint-reader/crosspoint-reader). See [What's changed from upstream](#whats-changed-from-upstream) for a summary of hardware-specific modifications.

---
## Hardware License

The hardware designs in `/hardware` are licensed under the
[CERN-OHL-W v2](hardware/LICENSE-HARDWARE).


## Hardware

| Component | Details |
|---|---|
| MCU | ESP32-C3 (4MB flash) |
| Display | Good Display 4.2" grayscale e-ink panel |
| Storage | SD card (books, cache, user settings) |
| Frontlight | TI TPS61169 PWM-controlled LED driver, 15V boost |
| Battery | 700mAh LiPo |
| USB | USB-C (charging + native USB via differential pair) |
| PCB | 4-layer, custom KiCad design (Rev. B) |

### PCB stack-up

```
F.Cu     — signal
GND      — full ground plane
PWR      — power distribution
B.Cu     — signal
```

Ground pour with stitching vias on both outer layers. EMI-sensitive boost converter traces kept compact.

### Power rails

| Rail | Source | Purpose |
|---|---|---|
| 3.4–4.2V | LiPo battery | System input |
| 3.3V | LDO | ESP32-C3, logic |
| 5V | USB-C input | Charging |
| ±20V | Boost converter | E-ink panel driver |
| 15V | Boost converter (TPS61169) | Frontlight LED driver |

### Power management

Three operating states optimized for battery life:

| State | Description | Wake latency |
|---|---|---|
| Active | Full speed, display updating | — |
| Sleep | Reduced CPU clock, peripherals idle | Fast |
| Deep sleep | µA range consumption, triggered by power button | Slow |

The device enters Sleep automatically after a display update completes. Deep sleep is only used when powered off via the power button.

### Controls

| Button | Function |
|---|---|
| Left / Right / Back / Confirm | Context-sensitive navigation (current function shown on display) |
| Power button | Short press: page turn / Long press: power on/off |

---

## Firmware

Based on [Crosspoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) with modifications for custom hardware and 4MB flash constraint.

### What's changed from upstream

- **HalFrontlight** — new HAL driver for PWM-controlled frontlight via TPS61169
- **HalPowerManager** — smaller tweaks
- **HalGPIO** — GPIO, button layout adapted to custom PCB
- **partitions_4mb.csv** — custom partition scheme to fit 4MB flash (replaces upstream 16MB default)
- **platformio.ini** — 4MB build configuration
- **Display refresh** — modified refresh handling for Good Display 4.2" panel compatibility

See [`docs/4mb-build/`](docs/4mb-build/) for detailed documentation on the 4MB build process and known issues.

---

## Repository structure

```

```

---

## Build

### Requirements

- [PlatformIO](https://platformio.org/)
- ESP32-C3 board with 4MB flash

### Steps

```bash
git clone https://github.com/IS7V4N/ESP32_E-Reader.git
cd ESP32_E-Reader
git submodule update --init --recursive
~/.platformio/penv/bin/pio run -e 4mb -t upload
```

---

## Known issues

See [`docs/4mb-build/known-bugs.md`](docs/4mb-build/known-bugs.md).

---

## License

Firmware modifications follow the license of the upstream [Crosspoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) project.
