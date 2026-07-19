# Build Guide

This guide walks through assembling the ESP32 E-Reader from a bare PCB to a working device: soldering, enclosure printing, firmware flashing, and first boot.

---

## 1. Get the PCB made

Order the PCB from [JLCPCB](https://jlcpcb.com/) using the Gerbers in [`Hardware/PCB/production`](../PCB/production). Components are sourced from [LCSC](https://www.lcsc.com/) — use the ready-to-upload BOM at [`Hardware/BOM/LCSC_ESP32_EReader_BOM - Ereader_bom-2.csv`](../BOM/LCSC_ESP32_EReader_BOM%20-%20Ereader_bom-2.csv) for parts ordering, and the interactive BOM at [`Hardware/BOM/BOM for building/ibom.html`](<../BOM/BOM for building/ibom.html>) to locate components on the board during assembly.

You have two options:

- **JLCPCB assembly (SMT)** — JLCPCB can assemble the board for you. This is the easiest route, but you'll still need to hand-solder the battery connector yourself, since the battery isn't part of the assembly service.
- **Self-assembly (reflow)** — order bare PCBs and solder paste/stencil, and reflow the board yourself.

Hand-soldering the whole board with an iron instead of reflow is possible, but the components are small enough that it requires real micro-soldering experience and proper tools (fine-tip iron, flux, hot air for rework). Reflow is strongly recommended unless you're already comfortable at that scale.

---

## 2. Tools and materials

**For assembly (if self-reflowing):**
- Solder paste (used: Sn63Pb37, Mechanic brand)
- Hot plate (100×100 mm) for reflow
- Soldering iron/station — for touch-up and fixing solder bridges
- Solder wire, flux
- Tweezers for component placement
- Multimeter — for continuity/short checks and voltage verification

**For final assembly:**
- A soldering iron (needed even with JLCPCB assembly, to attach the battery connector)
- Some tape, to hold the battery in place during fitting
- A 3D printer, for the enclosure
- A computer, to flash the firmware

No screws or glue are needed — the enclosure is a snap-fit design.

---

## 3. Print the enclosure

The enclosure files are in [`Hardware/3D CAD`](../3D%20CAD).

**Recommended material: PETG.** PLA is not recommended — under direct sunlight (e.g. reading outdoors, or left on a dashboard/windowsill) it can soften or warp due to heat. PETG has better impact and heat resistance, both useful for a handheld device.

---

## 4. Assemble the board

1. Reflow (or have JLCPCB assemble) the main PCB per the BOM above.
2. **Before connecting anything else, inspect the board and check the power rails with a multimeter for shorts.** Do this before the battery is anywhere near the board.
3. Solder on the battery connector. A **BT2 connector** was used on the prototype rather than soldering the battery leads directly to the board — this makes it easy to detach the battery later for storage, transport, or debugging.

> **⚠️ Battery polarity warning:** Double- and triple-check polarity before connecting or soldering the battery. Reversing polarity is destructive — it will very likely damage the board and/or the battery. Verify polarity with a multimeter against the connector footprint silkscreen before touching anything to the battery pads.

4. Once the connector is on, re-check the voltage rails for shorts *again* before actually plugging in the battery.

![Backplate removed, showing the battery connector](../Images/Backplate_off.jpg)

---

## 5. Flash the firmware

1. Connect the battery.
2. Connect the board to your computer via USB-C.
3. Flash the firmware — see the [main README](../../README.md#build) for the PlatformIO build/upload steps.
4. On first boot, an **SD card error** on screen is expected and normal if no SD card is inserted yet — it's not a fault.

---

## 6. Insert the SD card and boot

1. Insert an SD card (formatted, with your books — see the firmware docs for supported formats) into the slot.
2. Restart the device using the power button on the side: **long-press it twice** — the first press turns the device off, the second turns it back on.
3. If everything is working, you'll see the Crosspoint home menu on screen.

---

## 7. Done

Snap the enclosure shut and start reading.

---

## Troubleshooting / tips

- If the board doesn't power on, re-check the battery polarity and rail voltages before doing anything else — don't leave the battery connected while debugging shorts.
- If you see the SD card error persist after inserting a card, double check the card is formatted correctly and fully seated in the slot.

