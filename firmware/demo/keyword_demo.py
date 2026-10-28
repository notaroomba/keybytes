"""
Talks to a keyword over BLE: finds it, connects, runs the config protocol
over NUS and prints everything it sends back.

    python3 -m venv .venv && .venv/bin/pip install bleak
    .venv/bin/python keyword_demo.py            # scan, HELLO, SIDES?, KEYS?
    .venv/bin/python keyword_demo.py --map-a    # also MAP the first Keybit to 'a' and watch presses

On macOS the first write to RX needs an encrypted link, so the system pops
up a pairing request the first time.
"""
import argparse
import asyncio
import re
import sys
import time

from bleak import BleakClient, BleakScanner

NUS_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   # we write
NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   # keyword notifies
HID_SERVICE = "00001812-0000-1000-8000-00805f9b34fb"

t0 = time.monotonic()


def log(msg: str) -> None:
    print(f"[{time.monotonic() - t0:7.3f}] {msg}", flush=True)


class Lines:
    """Rebuilds \\n terminated lines from 20 byte notifications."""

    def __init__(self) -> None:
        self.buf = b""
        self.queue: asyncio.Queue[str] = asyncio.Queue()

    def feed(self, _sender, data: bytearray) -> None:
        self.buf += bytes(data)
        while b"\n" in self.buf:
            line, self.buf = self.buf.split(b"\n", 1)
            text = line.decode(errors="replace").rstrip("\r")
            log(f"<- {text}")
            self.queue.put_nowait(text)

    async def until(self, pred, timeout: float = 3.0) -> list[str]:
        got = []
        end = time.monotonic() + timeout
        while True:
            left = end - time.monotonic()
            if left <= 0:
                raise TimeoutError(f"no matching reply, got {got}")
            line = await asyncio.wait_for(self.queue.get(), left)
            got.append(line)
            if pred(line):
                return got


async def send(client: BleakClient, cmd: str) -> None:
    log(f"-> {cmd}")
    data = (cmd + "\n").encode()
    for i in range(0, len(data), 20):   # the protocol says 20 byte chunks
        await client.write_gatt_char(NUS_RX, data[i:i + 20], response=True)


async def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--scan-time", type=float, default=10)
    ap.add_argument("--map-a", action="store_true", help="MAP the first Keybit to 'a' (key=04) and watch PRESS")
    ap.add_argument("--watch", type=float, default=30, help="seconds to watch PRESS events with --map-a")
    args = ap.parse_args()

    log(f"scanning {args.scan_time:.0f} s for a device named Keyword...")
    found = await BleakScanner.discover(timeout=args.scan_time, return_adv=True)
    keyword = None
    for dev, adv in found.values():
        name = adv.local_name or dev.name or ""
        if name.startswith("Keyword"):
            log(f"found {name!r} {dev.address} rssi={adv.rssi} dBm services={adv.service_uuids}")
            keyword = keyword or dev
    if keyword is None:
        log(f"no Keyword advertising ({len(found)} other devices seen)")
        return 1

    lines = Lines()
    async with BleakClient(keyword, timeout=20) as client:
        log(f"connected, mtu={client.mtu_size}")
        services = {s.uuid.lower(): s for s in client.services}
        for uuid, svc in services.items():
            log(f"service {uuid} {svc.description}")
        if NUS_SERVICE not in services:
            log("NUS missing")
            return 1
        if HID_SERVICE in services:
            log("HID service present")

        await client.start_notify(NUS_TX, lines.feed)
        await send(client, "HELLO")
        await lines.until(lambda l: l.startswith("KEYWORD"), timeout=30)  # first write may wait on pairing
        await send(client, "SIDES?")
        await lines.until(lambda l: l.startswith("SIDES"))
        await send(client, "KEYS?")
        keys = await lines.until(lambda l: l == "KEYS END")
        uids = [m.group(1) for l in keys if (m := re.match(r"KEY\+ uid=([0-9A-F]{24})", l))]

        if args.map_a:
            if not uids:
                log("no Keybit attached, skipping MAP")
            else:
                uid = uids[0]
                await send(client, f"MAP uid={uid} mods=00 key=04")
                await lines.until(lambda l: l.startswith("OK MAP"))
                await send(client, f"MAP? uid={uid}")
                await lines.until(lambda l: l.startswith("MAP"))
                log(f"press the Keybit {uid} now, watching {args.watch:.0f} s")
                await asyncio.sleep(args.watch)   # PRESS lines print as they arrive

        await client.stop_notify(NUS_TX)
    log("done")
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
