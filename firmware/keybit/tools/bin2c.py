#!/usr/bin/env python3
"""Turn keybit.bin into a C file the keyword firmware links in."""
import re
import sys

src, dst, main_c = sys.argv[1], sys.argv[2], sys.argv[3]
data = open(src, "rb").read()
version = int(re.search(r"#define\s+FW_VERSION\s+(\d+)", open(main_c).read()).group(1))

# The last 64-byte page holds the Keybit's assigned I2C address.
if len(data) > 16 * 1024 - 64:
    sys.exit(f"keybit image is {len(data)} bytes and would overwrite the config page")

lines = []
for i in range(0, len(data), 16):
    lines.append("  " + ", ".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",")

open(dst, "w").write(
    "/* Generated from firmware/keybit by `make image` there. Do not edit. */\n"
    '#include "keybit_image.h"\n\n'
    f"const uint32_t keybit_image_len = {len(data)};\n"
    f"const uint8_t keybit_image_version = {version};\n"
    "const uint8_t keybit_image[] __attribute__((aligned(4))) = {\n"
    + "\n".join(lines) + "\n};\n"
)
