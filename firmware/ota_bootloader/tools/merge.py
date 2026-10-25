"""
Builds the one-time install image: the OTA bootloader at 0x08000000, 0xFF
up to the app, the app at 0x08007000.

    merge.py BOOT.bin APP.bin 0x7000 OUT.bin           write OUT.bin
    merge.py --check BOOT.bin APP.bin 0x7000 OUT.bin   verify it byte by byte

The check also runs the bootloader's own validity test on the app (the
word at +0x140 must point at 0x94448A29 inside the image) and makes sure
nothing reaches the keymap page at 0x080CF000.
"""
import struct
import sys

FLASH_BASE = 0x08000000
KEYMAP_PAGE = 0x080CF000   # SFSA - 1 with the v1.24.0 full BLE stack at 0x080D0000
MAGIC = 0x94448A29
TAG_OFFSET = 0x140


def fail(msg: str) -> None:
    sys.exit(f"merge.py: {msg}")


def check_app(app: bytes, base: int) -> None:
    if len(app) < TAG_OFFSET + 4:
        fail("app image too small")
    sp, reset = struct.unpack_from("<II", app, 0)
    if not 0x20000000 <= sp <= 0x20030000:
        fail(f"app initial SP {sp:#x} is not in SRAM1")
    if not base <= (reset & ~1) < base + len(app):
        fail(f"app reset vector {reset:#x} is outside the app")
    (tag_addr,) = struct.unpack_from("<I", app, TAG_OFFSET)
    if not base <= tag_addr <= base + len(app) - 4:
        fail(f"app tag address {tag_addr:#x} is outside the image (TAG_OTA_START missing?)")
    (magic,) = struct.unpack_from("<I", app, tag_addr - base)
    if magic != MAGIC:
        fail(f"no {MAGIC:#x} at {tag_addr:#x}: the bootloader would refuse this app")
    if tag_addr - base + 4 != len(app) and tag_addr - base + 8 < len(app):
        fail(f"tag at {tag_addr:#x} is not at the end of the image ({base + len(app):#x})")


def main() -> None:
    args = sys.argv[1:]
    check = args and args[0] == "--check"
    if check:
        args = args[1:]
    if len(args) != 4:
        fail(__doc__.strip())
    boot = open(args[0], "rb").read()
    app = open(args[1], "rb").read()
    offset = int(args[2], 0)

    if len(boot) > offset:
        fail(f"bootloader is {len(boot)} bytes, more than the {offset:#x} before the app")
    app_base = FLASH_BASE + offset
    if app_base + len(app) > KEYMAP_PAGE:
        fail(f"app ends at {app_base + len(app):#x}, past the keymap page {KEYMAP_PAGE:#x}")
    check_app(app, app_base)
    want = boot + b"\xff" * (offset - len(boot)) + app

    if not check:
        open(args[3], "wb").write(want)
        print(f"{args[3]}: {len(want)} bytes, bootloader {len(boot)} at {FLASH_BASE:#x}, "
              f"app {len(app)} at {app_base:#x}, ends {FLASH_BASE + len(want):#x}")
        return

    got = open(args[3], "rb").read()
    if got[:len(boot)] != boot:
        fail("bootloader bytes differ")
    if any(b != 0xFF for b in got[len(boot):offset]):
        fail("gap before the app is not all 0xFF")
    if got[offset:] != app:
        fail("app bytes differ")
    if got != want:
        fail("image differs")
    print(f"{args[3]}: verified, bootloader {len(boot)} B + 0xFF gap {offset - len(boot)} B "
          f"+ app {len(app)} B at {app_base:#x}")


if __name__ == "__main__":
    main()
