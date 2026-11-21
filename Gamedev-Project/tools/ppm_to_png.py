#!/usr/bin/env python3
"""
ppm_to_png.py - turn the game's --screenshot output into a PNG.

The game writes binary PPM (P6) because that needs no image library on the C++
side. Nothing on macOS opens PPM, so this converts it. Pure standard library,
no pip install.

    ./roary --floor 1 --screenshot shot.ppm --frames 2
    python3 tools/ppm_to_png.py shot.ppm shot.png
"""

import struct
import sys
import zlib


def read_ppm(path):
    with open(path, "rb") as handle:
        data = handle.read()

    if not data.startswith(b"P6"):
        raise SystemExit("%s is not a binary PPM (expected a P6 header)" % path)

    # The header is whitespace-separated tokens, and may carry # comments.
    fields = []
    offset = 2
    while len(fields) < 3:
        while offset < len(data) and data[offset:offset + 1].isspace():
            offset += 1
        if data[offset:offset + 1] == b"#":
            while offset < len(data) and data[offset:offset + 1] != b"\n":
                offset += 1
            continue
        start = offset
        while offset < len(data) and not data[offset:offset + 1].isspace():
            offset += 1
        fields.append(int(data[start:offset]))

    offset += 1   # the single whitespace byte before the pixel data
    width, height, maxval = fields
    if maxval != 255:
        raise SystemExit("only 8-bit PPM is supported (maxval was %d)" % maxval)

    expected = width * height * 3
    pixels = data[offset:offset + expected]
    if len(pixels) != expected:
        raise SystemExit("truncated: wanted %d pixel bytes, got %d"
                         % (expected, len(pixels)))
    return width, height, pixels


def write_png(path, width, height, pixels):
    def chunk(tag, payload):
        body = tag + payload
        return (struct.pack(">I", len(payload)) + body
                + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF))

    # PNG wants every scanline prefixed with its filter byte.
    stride = width * 3
    raw = bytearray()
    for row in range(height):
        raw.append(0)
        raw += pixels[row * stride:(row + 1) * stride]

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", header))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(raw), 6)))
        handle.write(chunk(b"IEND", b""))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: ppm_to_png.py <in.ppm> <out.png>")
    width, height, pixels = read_ppm(sys.argv[1])
    write_png(sys.argv[2], width, height, pixels)
    print("%s  %dx%d" % (sys.argv[2], width, height))


if __name__ == "__main__":
    main()
