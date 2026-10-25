# keybit firmware

Firmware for the Keybit's CH32V003F4U6, built with
[ch32v003fun](https://github.com/cnlohr/ch32v003fun). The keyword carries a
copy (`../Core/Src/keybit_image.c`) and flashes it onto any Keybit placed on
its programming studs, so normally you never flash a Keybit by hand.

## Build

Needs a RISC-V GCC (`brew install riscv64-elf-gcc`, or
`gcc-riscv64-unknown-elf` on Ubuntu). The first build clones ch32v003fun
into `ch32v003fun/` at a pinned commit.

```sh
cd firmware/keybit
make          # main.bin
make image    # regenerate ../Core/Src/keybit_image.c, then rebuild the keyword
make flash    # optional: flash directly with a WCH-LinkE
```

## What it does

Each side of a Keybit has its own SDA/SCL pair, so all four are bit-banged
I2C targets (`soft_i2c.c`); the main loop polls them for a START and serves
one transaction at a time, clock-stretching where it needs time.

| Bus | SDA | SCL | Side |
|-----|-----|-----|------|
| 1 | PC4 | PD2 | S |
| 2 | PC1 | PC2 | E |
| 3 | PD4 | PD5 | W |
| 4 | PA1 | PA2 | N |

The button is on PD6 (to GND, internal pull-up). PD1 stays SWIO so the
keyword can always reflash it.

## Addresses

The keyword hands out I2C addresses:

1. A Keybit with no address answers on **0x30**.
2. The keyword reads the 96-bit UID at 0x10. If several new Keybits answer
   at once, they arbitrate bit by bit (open-drain, a 0 wins) and the
   keyword gets the lowest UID cleanly; the others drop out.
3. The keyword writes that UID plus a free address to 0x20..0x2C. Only the
   matching Keybit takes it and saves it in the last flash page
   (0x08003FC0), so it keeps the address after power-off and reflashing.
4. Repeat until nobody answers on 0x30.

If two Keybits ever end up with the same address, the next UID read
arbitrates between them and the loser goes back to 0x30 for a new one.

## Registers

Reads and writes auto-increment from the pointer written first.

| Reg | Meaning |
|-----|---------|
| 0x00 | 0x4B magic |
| 0x01 | firmware version |
| 0x02-0x05 | UID bits 0..31 |
| 0x06 | key state, bit 0 = pressed |
| 0x07 | side this request came in on, `N`/`E`/`S`/`W` |
| 0x08 | current address, 0 if none |
| 0x10-0x1B | 96-bit UID, little endian |
| 0x20-0x2B | ASSIGN: UID to match (write) |
| 0x2C | ASSIGN: new address, 0 = forget. Writing this applies it. |
| 0x51 | our depth, 0xFF if we can't reach the keyword |
| 0x52 | events ready, at most 8 |
| 0x53-0x7A | up to 8 events of 5 bytes: UID bits 0..31, flags |
| 0x7B | (write) n: drop the first n events |

Event flags: bit 0 pressed, bit 1 heartbeat, bits 4-7 the origin Keybit's
depth.

## Relaying

Only Keybits touching the keyword are on its bus. Every other joint is a
private link between two Keybits, so key state hops inward along a tree:

- **Depth.** A Keybit the keyword talks to has depth 1. Any other Keybit
  takes one more than the shallowest neighbour that polls it. Depth only
  comes from being polled, so when a Keybit is pulled out everything behind
  it drops to "no depth" within 600 ms, with no loops.
- **Polling.** A Keybit with a depth polls each other side over the link
  protocol below, unless that neighbour polls it or is no deeper. It polls
  a known child every ~100 ms to keep its depth fresh, probes empty sides
  every ~100 ms, and polls at once when a child rings the doorbell.
- **Events.** Each Keybit queues its own key changes plus a heartbeat about
  once a second, and forwards its children's. The keyword drains the
  touching Keybits over I2C (`keybit_bus_poll_relay`, registers
  0x51-0x7B) into `keybit_remotes[]`, and drops a remote Keybit after 3 s
  without a heartbeat.
- **Safety.** A side where an I2C address shows up is the keyword's shared
  bus; a Keybit never starts a link frame or rings there. It also listens
  for 600 ms after power-up before driving anything.

## Link protocol

Keybit-to-Keybit links use the same two pins as I2C, renamed DT (SDA) and
CK (SCL), with a lighter protocol (`link.c`):

1. **Idle and handshake are open-drain**, on the 10k pull-ups, because
   either end may pull a line and an unknown side could be the keyword's
   bus.
   - *Request:* the parent pulls CK low. On a side not yet known to be a
     link, the child only answers once CK has been low for 50 us with DT
     high, which never happens on I2C.
   - *Doorbell:* a child with events holds DT low (CK high) towards its
     parent, which polls it straight away.
   - *Ack:* the child pulls DT low; the parent releases CK, the child
     releases DT, and both lines high starts the frame.
2. **The frame is push-pull.** The parent drives CK; whoever is sending
   drives DT, and at each turnaround one end lets go 1 us before the other
   drives, so outputs never fight. Half a clock period is 0.5 us, so about
   1 Mbit/s, MSB first. The parent never waits on the child mid-frame, so
   a Keybit pulled out mid-frame can't hang it; a child that falls behind
   just fails the CRC.

| Direction | Bytes |
|---|---|
| parent -> child | 0xA5, parent depth |
| child -> parent | child depth, n (0-8), n x 5 event bytes, CRC-8 (poly 0x07) |
| parent -> child | kept, ~kept |

The child drops the first `kept` events. A frame with one event takes about
120 us including the handshake, so a key press moves one hop in well under
a millisecond. If frames fail on real hardware, raise `HALF` in `link.c`
first.

Remote Keybits are identified by the low 32 bits of their UID.
