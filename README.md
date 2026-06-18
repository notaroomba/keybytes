<h1 align="center">
  <br>
  <a href="https://notaroomba.dev"><img src="https://raw.githubusercontent.com/notaroomba/keybytes/main/assets/banner.png" alt="Keybytes" width="600"></a>
  <br>
</h1>

<h4 align="center">
A truly modular keyboard — rearrange every key, any time, and let the board figure itself out.
</h4>

<div align="center">

![KiCad](https://img.shields.io/badge/kicad-%2300578F.svg?style=for-the-badge&logo=kicad&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white)
![RISC-V](https://img.shields.io/badge/RISC--V-283272?style=for-the-badge&logo=riscv&logoColor=white)
![Onshape](https://img.shields.io/badge/onshape-%23217346.svg?style=for-the-badge&logo=onshape&logoColor=white)
![Blender](https://img.shields.io/badge/blender-%23F5792A.svg?style=for-the-badge&logo=blender&logoColor=white)

</div>

<p align="center">
  <a href="#key-features">Key Features</a> •
  <a href="#how-it-works">How It Works</a> •
  <a href="#hardware">Hardware</a> •
  <a href="#case--keycaps">Case &amp; Keycaps</a> •
  <a href="#firmware">Firmware</a> •
  <a href="#repository-layout">Layout</a> •
  <a href="#credits">Credits</a> •
  <a href="#license">License</a>
</p>

<img src="https://stasis.hackclub-assets.com/images/1775446958195-0ybbic.png" alt="Keybytes" width="800"/>

## Key Features

- **Truly modular** — every key is its own self-contained board. Pop them off, rearrange the layout, and snap them back wherever you want.
- **Magnetic keycaps** — caps hold on with embedded **2×0.5mm magnets** and latch onto the switch's side rails. No tools, no soldering.
- **Pogo-pin interconnect** — keys link to the main board (and each other) through spring-loaded pogo pins and pads, so there's nothing to plug in.
- **Smart per-key MCU** — each module runs a cheap **CH32V003 RISC-V** microcontroller and reports over a shared **I²C** bus.
- **Bluetooth + USB main board** — the central board is built around an **STM32WB55RGV6** (Cortex-M4 + radio) with USB-HID and Bluetooth LE.
- **In-system programming** — the main board can flash the keybit MCUs directly through pogo pins (single-wire `BB_SWIO`).
- **External QUADSPI flash** for layouts, graphics, and key data.
- **RGB status LED** for connection and mode feedback.
- **Battery support** for going wireless.

## How It Works

Keybytes is split into two kinds of board:

| Board | Role | Brains |
|-------|------|--------|
| **Keybit** | A single, swappable key module | CH32V003 (RISC-V) |
| **Keyword** | The central hub every key plugs into | STM32WB55RGV6 (Bluetooth + USB) |

Each **keybit** carries its own switch, RGB LED, and microcontroller. Instead of a fixed switch matrix, every keybit talks to the **keyword** over a shared **I²C** bus through pogo-pin contacts. Because each key is individually addressed, the board doesn't care *where* a key physically sits — you can pull keys off and rearrange the whole layout at will, and the keyword maps it back to keystrokes over USB-HID or Bluetooth.

The keyword can also reprogram every keybit in place over its single-wire debug line, so firmware updates don't mean desoldering anything.

## Hardware

Designed in [KiCad](https://www.kicad.org/). The repo holds the full schematics, PCB layouts, 3D models, and JLCPCB-ready fabrication outputs for both boards.

### Keybit

The per-key module: a tiny board with a hot-swap switch footprint, an RGB LED, the CH32V003, I²C pull-ups, and pogo pads for power, data, and programming.

<img src="https://stasis.hackclub-assets.com/images/1775426542816-taq9ky.png" alt="Keybit board" width="800"/>
<img src="https://stasis.hackclub-assets.com/images/1775416826793-5lrrwe.png" alt="Keybit routing" width="800"/>

### Keyword

The main board / hub. Built around the STM32WB55RGV6 for Bluetooth LE and USB, with external QUADSPI flash, an RGB status LED, battery support, and the pogo-pin field that the keybits connect into. It also drives the single-wire programming line for in-system flashing of the keys.

<img src="https://stasis.hackclub-assets.com/images/1775347277075-v4u04k.png" alt="Keyword routing" width="800"/>
<img src="https://stasis.hackclub-assets.com/images/1775419082878-wwvq7s.png" alt="Keyword with pogo programming and flash" width="800"/>

## Case & Keycaps

Cases, keycaps, and clamps are modeled in [OnShape](https://www.onshape.com/) and exported as STEP files in [`/cad`](cad). Keycaps print with internal pockets for the magnets and clip directly onto the switch side latches — no glue required.

<img src="https://stasis.hackclub-assets.com/images/1775792561658-u0t4fh.png" alt="Keycap design" width="800"/>
<img src="https://stasis.hackclub-assets.com/images/1775792667844-tdun0b.png" alt="Keycap magnet pockets and latch" width="800"/>
<img src="https://stasis.hackclub-assets.com/images/1775793071456-4ktbyd.png" alt="Printed keycap" width="800"/>

## Firmware

The keyword firmware lives in [`/firmware`](firmware) and targets the **STM32WB55** using the STM32 HAL, generated with [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) and built with the included `Makefile` / [STM32-for-VSCode](https://marketplace.visualstudio.com/items?itemName=bmd.stm32-for-vscode) config.

Key peripherals configured:

- **USB** device (HID)
- **I²C3** — the bus to all the keybits
- **QUADSPI** — external flash
- **RF** — Bluetooth LE radio
- **RGB** status LED + single-wire keybit programming (`BB_SWIO`)

```bash
cd firmware
make            # build
make flash      # flash over OpenOCD (see openocd.cfg)
```

## Repository Layout

```
keybytes/
├── assets/          # banner + images
├── blender/         # .blend scene, .glb / .pcb3d models, renders
├── cad/             # OnShape STEP exports (case, keycap, clamp) + glb converter
├── firmware/        # STM32WB55 firmware (CubeMX + Makefile)
├── hardware/
│   ├── keybit/      # KiCad project for the per-key module
│   ├── keyword/     # KiCad project for the main board (top/bottom)
│   └── lib/         # shared symbol / footprint / 3D libraries
└── BOM.csv          # bill of materials
```

## Bill of Materials

The interconnect and magnet parts that make the modular system work — see [`BOM.csv`](BOM.csv) for sources:

| Item | Price | Source |
|------|-------|--------|
| Magnets 2×0.5mm (1000 pcs) | $8.83 | [AliExpress](https://es.aliexpress.com/item/1005009461785489.html) |
| Pogo Pads 1u | $0.04 | [LCSC](https://www.lcsc.com/product-detail/C2826547.html) |
| Pogo Pins 1u | $0.09 | [LCSC](https://www.lcsc.com/product-detail/C42419351.html) |
| Pogo Headers 4P | $1.75 | [LCSC](https://www.lcsc.com/product-detail/C5296819.html) |
| Pogo Headers 2P | $0.75 | [LCSC](https://www.lcsc.com/product-detail/C5296818.html) |
| Pogo Headers 2P (longer) | $0.87 | [LCSC](https://www.lcsc.com/product-detail/C5296817.html) |

## Credits

This project uses:

- [KiCad](https://www.kicad.org/) for the schematics and PCBs
- [OnShape](https://www.onshape.com/) for the cases and keycaps
- [Blender](https://www.blender.org/) for 3D renders
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) for the keyword firmware
- [WCH CH32V003](https://www.wch-ic.com/products/CH32V003.html) — the RISC-V brain in every keybit
- [KiCad Fabrication Toolkit](https://github.com/bennymeg/Fabrication-Toolkit) for JLCPCB production files

## You may also like...

- [Ember](https://github.com/NotARoomba/ember) — A USB-C powered reflow hotplate with Bluetooth
- [Cyberboard](https://github.com/NotARoomba/Cyberboard) — A Raspberry Pi Pico-sized STM32 dev board with Bluetooth
- [Trace](https://github.com/NotARoomba/Trace) — A comprehensive PCB ruler with reference footprints
- [Linea](https://github.com/NotARoomba/Linea) — An EMR tablet

## License

MIT

---

> [notaroomba.dev](https://notaroomba.dev) &nbsp;&middot;&nbsp;
> GitHub [@NotARoomba](https://github.com/NotARoomba) &nbsp;&middot;&nbsp;
> Built for [Hack Club](https://hackclub.com)
