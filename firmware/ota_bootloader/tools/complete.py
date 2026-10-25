"""
Builds the one-flash images: the OTA bootloader at 0x08000000, the app at
0x08007000 and ST's CPU2 BLE stack at 0x080D0000, as three separate
segments. The gaps aren't in the files, so a flash never touches the keymap
page or anything of FUS.

    complete.py BOOT.bin APP.bin STACK.bin OUT_BASE    writes OUT_BASE.hex and OUT_BASE.dfu
    complete.py --check BOOT.bin APP.bin STACK.bin OUT_BASE

The stack must be stm32wb5x_BLE_Stack_full_fw.bin 1.24.0 (SHA-256 below):
ST's SLA0044 keeps it out of the repo, so the outputs aren't committed.
--check decodes both files and compares them with the three images.
"""
import hashlib
import struct
import sys
import zlib

from merge import FLASH_BASE, check_app

APP_ADDR = 0x08007000
STACK_ADDR = 0x080D0000
STACK_SHA256 = "b7855d9ce0b012fabff5782a574a3038e6e5b1889c5c9bcdb06c2cab2201c05d"
KEYMAP_PAGE = STACK_ADDR - 0x1000    # SFSA - 1 once the stack is installed
FUS_ONLY_SFSA = 0x080F4000           # SFSA on a chip with FUS alone (1 MB part)


def fail(msg: str) -> None:
    sys.exit(f"complete.py: {msg}")


def pad8(data: bytes) -> bytes:
    """The ROM DFU writes flash in 64-bit double words."""
    return data + b"\xff" * (-len(data) % 8)


def segments(boot: bytes, app: bytes, stack: bytes) -> list:
    if hashlib.sha256(stack).hexdigest() != STACK_SHA256:
        fail("stack is not stm32wb5x_BLE_Stack_full_fw.bin 1.24.0 (SHA-256 differs)")
    if FLASH_BASE + len(boot) > APP_ADDR:
        fail(f"bootloader is {len(boot)} bytes, past the app at {APP_ADDR:#x}")
    if APP_ADDR + len(app) > KEYMAP_PAGE:
        fail(f"app ends at {APP_ADDR + len(app):#x}, past the keymap page {KEYMAP_PAGE:#x}")
    if STACK_ADDR + len(stack) > FUS_ONLY_SFSA - 0x1000:
        fail("stack reaches the keymap page of a chip without a stack")
    check_app(app, APP_ADDR)
    return [(FLASH_BASE, pad8(boot)), (APP_ADDR, pad8(app)), (STACK_ADDR, pad8(stack))]


def ihex(segs: list) -> str:
    def rec(kind: int, addr: int, data: bytes) -> str:
        body = bytes([len(data), addr >> 8 & 0xFF, addr & 0xFF, kind]) + data
        return ":" + (body + bytes([-sum(body) & 0xFF])).hex().upper() + "\n"

    out = []
    for base, data in segs:
        upper = None
        for off in range(0, len(data), 16):
            a = base + off
            if a >> 16 != upper:
                upper = a >> 16
                out.append(rec(4, 0, struct.pack(">H", upper)))
            out.append(rec(0, a & 0xFFFF, data[off:off + 16]))
    out.append(rec(1, 0, b""))
    return "".join(out)


def parse_ihex(text: str) -> list:
    """Back to [(addr, bytes)], joining contiguous records."""
    segs, upper = [], 0
    for line in text.splitlines():
        b = bytes.fromhex(line[1:])
        if line[0] != ":" or sum(b) & 0xFF or b[0] != len(b) - 5:
            fail(f"bad hex record {line}")
        kind, addr, data = b[3], b[1] << 8 | b[2], b[4:-1]
        if kind == 4:
            upper = struct.unpack(">H", data)[0]
        elif kind == 0:
            a = upper << 16 | addr
            if segs and segs[-1][0] + len(segs[-1][1]) == a:
                segs[-1] = (segs[-1][0], segs[-1][1] + data)
            else:
                segs.append((a, data))
        elif kind == 1:
            return segs
        else:
            fail(f"unexpected hex record type {kind}")
    fail("hex has no end record")


def dfuse(segs: list) -> bytes:
    """ST's DfuSe file (UM0391): one target, alt 0, one element per segment."""
    elems = b"".join(struct.pack("<II", a, len(d)) + d for a, d in segs)
    target = (b"Target" + struct.pack("<BI", 0, 1) + b"Internal Flash".ljust(255, b"\0")
              + struct.pack("<II", len(elems), len(segs)) + elems)
    body = b"DfuSe" + struct.pack("<BIB", 1, 11 + len(target), 1) + target
    suffix = struct.pack("<HHHH", 0xFFFF, 0xDF11, 0x0483, 0x011A) + b"UFD" + bytes([16])
    data = body + suffix
    return data + struct.pack("<I", zlib.crc32(data) ^ 0xFFFFFFFF)


def parse_dfuse(f: bytes) -> list:
    if f[:5] != b"DfuSe" or f[5] != 1 or f[-8:-5] != b"UFD":
        fail("not a DfuSe file")
    if struct.unpack("<I", f[-4:])[0] != zlib.crc32(f[:-4]) ^ 0xFFFFFFFF:
        fail("DfuSe CRC wrong")
    size, ntargets = struct.unpack_from("<IB", f, 6)
    if size != len(f) - 16 or ntargets != 1 or f[11:17] != b"Target":
        fail("DfuSe header wrong")
    tsize, nelem = struct.unpack_from("<II", f, 11 + 6 + 5 + 255)
    p, segs = 11 + 274, []
    for _ in range(nelem):
        a, n = struct.unpack_from("<II", f, p)
        segs.append((a, f[p + 8:p + 8 + n]))
        p += 8 + n
    if p != 11 + 274 + tsize or p != size:
        fail("DfuSe element sizes don't add up")
    return segs


def main() -> None:
    args = sys.argv[1:]
    check = bool(args) and args[0] == "--check"
    if check:
        args = args[1:]
    if len(args) != 4:
        fail(__doc__.strip())
    boot, app, stack = (open(p, "rb").read() for p in args[:3])
    segs = segments(boot, app, stack)
    hex_path, dfu_path = args[3] + ".hex", args[3] + ".dfu"

    if not check:
        open(hex_path, "w", newline="\n").write(ihex(segs))
        open(dfu_path, "wb").write(dfuse(segs))
        for a, d in segs:
            print(f"  {a:#010x}-{a + len(d) - 1:#010x}  {len(d)} B")
        print(f"{hex_path}, {dfu_path}: 3 segments, gaps not written")
        return

    for name, got in (("hex", parse_ihex(open(hex_path).read())),
                      ("dfu", parse_dfuse(open(dfu_path, "rb").read()))):
        if got != segs:
            fail(f"{name} doesn't decode to the three images")
        print(f"{name}: verified, " + ", ".join(f"{len(d)} B at {a:#x}" for a, d in got))


if __name__ == "__main__":
    main()
