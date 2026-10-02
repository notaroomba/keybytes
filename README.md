<h1 align="center">
  <br>
  <a href="https://notaroomba.dev"><img src="https://raw.githubusercontent.com/notaroomba/keybytes/main/assets/banner.png" alt="Keybytes" width="600"></a>
  <br>
</h1>

<h4 align="center">
A truly modular keyboard with rearrangeable keys, magnetic keycaps and Bluetooth!
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
  <a href="#pcb">PCB</a> •
  <a href="#case-and-keycaps">Case &amp; Keycaps</a> •
  <a href="#credits">Credits</a> •
  <a href="#license">License</a>
</p>

<img src="assets/render.png" alt="Keybytes Render" width="800"/>

## Key Features

- **Rearrangeable layout** where every key is its own 1u PCB
- **CH32V003** RISC-V microcontroller on every key
- **STM32WB55RGV6** main board with Bluetooth and USB
- **Pogo pins on all 4 sides** of every key for power and I2C
- **Magnetic keycaps** with 2mm x 0.5mm magnets
- **Key programming** from the main board over pogo pins
- **2MB QSPI flash** (W25Q16JV) to store the key firmware
- **Battery charging** with the BQ25895 and solder pads for a 1S LiPo
- **Buck-Boost converter** (TPS631000)
- **RGB status LED**
- **USB-C** with ESD protection
- **Ceramic Bluetooth antenna**

## PCB

Designed in KiCad! There are two boards, the Keybit and the Keyword, and both are 1u (19.05mm x 19.05mm).

### Keybit

A single key with a Cherry MX switch and a CH32V003. Every side has pogo pins for 3.3V, GND and I2C so it can connect to the keyword or to other keybits, and the pads on the corners are for programming.

**Schematic:**

<img src="assets/journal/1775426460134-qt999o.png" alt="Keybit Schematic" width="800"/>

**Layout:**

<img src="assets/journal/1775416826793-5lrrwe.png" alt="Keybit Layout" width="800"/>

### Keyword

The main board that the keybits connect to. It is split into two PCBs that stack on top of each other. The top one has the STM32WB55RGV6, the antenna and the flash, and the bottom one has the USB-C, the charger and the buck-boost converter. The 3 pogo pins on the top are for programming the keybits.

**Schematic:**

<img src="assets/journal/1775426515419-d7ntz6.png" alt="Keyword Schematic" width="800"/>

**Layout:**

<img src="assets/journal/1775347277075-v4u04k.png" alt="Keyword Layout" width="800"/>

**3D View:**

<img src="assets/journal/1775426542816-taq9ky.png" alt="Keyword 3D View" width="800"/>
<img src="assets/journal/1775426579651-lg5zme.png" alt="Keyword 3D View" width="800"/>

## Case and Keycaps

Custom designed cases and keycaps in OnShape, the STEP files are in the [`cad`](cad) folder.

<img src="assets/journal/1781815993601-r86dm5.png" alt="Case" width="800"/>

The keycaps have internal holes for the magnets and latch on to the side latches of the key switch.

<img src="assets/journal/1775792561658-u0t4fh.png" alt="Keycap Design" width="800"/>
<img src="assets/journal/1775792667844-tdun0b.png" alt="Keycap Latch" width="800"/>
<img src="assets/journal/1775793071456-4ktbyd.png" alt="Printed Keycap" width="400"/>

## Credits

This project uses:

- [KiCad](https://www.kicad.org/)
- [OnShape](https://www.onshape.com/) for case design
- [Blender](https://www.blender.org/) for 3D renders
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
- [KiCad Fabrication Toolkit](https://github.com/bennymeg/Fabrication-Toolkit) for the JLCPCB files
- [Hack Club Stasis](https://stasis.hackclub.com/)

## You may also like...

- [Ember](https://github.com/NotARoomba/ember) - A USB-C powered reflow hotplate with Bluetooth
- [Cyberboard](https://github.com/NotARoomba/Cyberboard) - A Raspberry Pi Pico-sized STM32 development board with Bluetooth
- [Trace](https://github.com/NotARoomba/Trace) - A comprehensive PCB ruler with reference footprints
- [Linea](https://github.com/NotARoomba/Linea) - An EMR tablet

## License

MIT

---

> [notaroomba.dev](https://notaroomba.dev) &nbsp;&middot;&nbsp;
> GitHub [@NotARoomba](https://github.com/NotARoomba) &nbsp;&middot;&nbsp;
