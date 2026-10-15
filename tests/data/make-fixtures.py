#!/usr/bin/env python3
"""Writes the picture files the 0005 tests open (tests/io/test-picture-file.c). Run by hand when a fixture must change,
then commit the outputs: python3 tests/data/make-fixtures.py. Needs Pillow; the app itself never uses it.

The pattern must stay in step with the C tests: rgba (x, y) below is `pattern` there."""

import os
import struct
import sys

from PIL import Image

out = os.path.dirname(os.path.abspath(__file__))
W, H = 7, 5


def rgba(x, y):
    """Red grows with x, green with y; alpha: 0 on (x + y) % 4 == 0, 128 where x % 3 == 0, else 255."""
    a = 0 if (x + y) % 4 == 0 else (128 if x % 3 == 0 else 255)
    return (30 * x + 20, 40 * y + 10, 100 + x + y, a)


def path(name):
    return os.path.join(out, name)


def pattern_image(mode):
    im = Image.new("RGBA", (W, H))
    for y in range(H):
        for x in range(W):
            im.putpixel((x, y), rgba(x, y))
    return im if mode == "RGBA" else im.convert("RGB")


# Formats with transparency, same pixels.
pattern_image("RGBA").save(path("rgba.png"))
pattern_image("RGBA").save(path("rgba.tiff"))
pattern_image("RGBA").save(path("rgba.webp"), lossless=True, exact=True)
# BMP: 24 bit, and 32 bit with an alpha channel that opening must ignore.
pattern_image("RGB").save(path("opaque.bmp"))
pattern_image("RGBA").save(path("rgba.bmp"))
# A named-wrong PNG.
pattern_image("RGBA").save(path("named.dat"), format="PNG")

# JPEG: one flat color, so the lossy result is predictable.
Image.new("RGB", (16, 16), (200, 100, 50)).save(path("flat.jpg"), quality=95, subsampling=0)

# Gray, indexed and 16-bit PNG: expected as RGBA by the tests.
gray = Image.new("L", (W, H))
for y in range(H):
    for x in range(W):
        gray.putpixel((x, y), 10 * x + 20 * y)
gray.save(path("gray.png"))
indexed = Image.new("P", (W, H))
indexed.putpalette([255, 0, 0, 0, 255, 0, 0, 0, 255, 10, 20, 30] + [0] * (768 - 12))
for y in range(H):
    for x in range(W):
        indexed.putpixel((x, y), (x + y) % 4)
indexed.save(path("indexed.png"))
g16 = Image.new("I;16", (W, H))
for y in range(H):
    for x in range(W):
        g16.putpixel((x, y), (10 * x + 20 * y) * 257)
g16.save(path("gray16.png"))

# GIF with a transparent color, and an animated one (red, then green).
gif = Image.new("P", (W, H))
gif.putpalette([255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0] + [0] * (768 - 12))
for y in range(H):
    for x in range(W):
        gif.putpixel((x, y), 3 if (x + y) % 4 == 0 else (x + y) % 3)
gif.save(path("transparent.gif"), transparency=3)
f1 = Image.new("RGB", (6, 4), (255, 0, 0))
f2 = Image.new("RGB", (6, 4), (0, 255, 0))
f1.save(path("anim.gif"), save_all=True, append_images=[f2], duration=100, loop=0)

# Orientation: a 32 x 16 JPEG of four solid 16 x 8 blocks, tagged with each of the eight EXIF values. Blocks fall on
# the 8 x 8 units of a JPEG without chroma subsampling, so the colors do not bleed.
blocks = Image.new("RGB", (32, 16))
for (bx, by), color in {(0, 0): (255, 0, 0), (1, 0): (0, 255, 0), (0, 1): (0, 0, 255), (1, 1): (255, 255, 0)}.items():
    blocks.paste(color, (16 * bx, 8 * by, 16 * bx + 16, 8 * by + 8))
for tag in range(1, 9):
    exif = Image.Exif()
    exif[0x0112] = tag
    blocks.save(path("orient-%d.jpg" % tag), quality=100, subsampling=0, exif=exif.tobytes())

# Sizes: at and over the limit of 10000 (one color, so the files stay small), and a wide JPEG tagged "rotate 90".
Image.new("L", (10000, 1), 7).save(path("w10000.png"))
Image.new("L", (1, 10000), 7).save(path("h10000.png"))
Image.new("L", (10001, 1), 7).save(path("w10001.png"))
Image.new("L", (1, 10001), 7).save(path("h10001.png"))
Image.new("L", (10001, 10001), 7).save(path("big10001.png"), compress_level=9)
exif = Image.Exif()
exif[0x0112] = 6
Image.new("RGB", (10001, 3), (10, 20, 30)).save(path("wide-rot6.jpg"), quality=90, exif=exif.tobytes())
# A picture that decodes fine but is big enough for the memory-failure test (36 million pixels).
Image.new("L", (6000, 6000), 9).save(path("flat6000.png"), compress_level=9)

# Not pictures.
with open(path("text.png"), "w") as f:
    f.write("This is not a PNG file.\n")
open(path("empty.png"), "w").close()
with open(path("vector.svg"), "w") as f:
    f.write('<svg xmlns="http://www.w3.org/2000/svg" width="4" height="4"><rect width="4" height="4"/></svg>\n')

# Broken files for the header sniffer (io test sniff_refuses_broken_files). Each is a few bytes.
png_head = open(path("rgba.png"), "rb").read()
open(path("cut-header.png"), "wb").write(png_head[:20])                         # cut before the size is complete
open(path("cut-data.jpg"), "wb").write(open(path("orient-1.jpg"), "rb").read()[:400])   # the size is there, the data is not


def png_header_only(width, height):
    """The signature and an IHDR chunk that states width x height, and nothing after it."""
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    import zlib
    chunk = b"IHDR" + ihdr
    return b"\x89PNG\r\n\x1a\n" + struct.pack(">I", len(ihdr)) + chunk + struct.pack(">I", zlib.crc32(chunk))


open(path("claims-10000.png"), "wb").write(png_header_only(10000, 10000))   # a legal size in a file of 33 bytes
open(path("claims-40000.png"), "wb").write(png_header_only(40000, 40000))   # an illegal size in a file of 33 bytes
# A JPEG that is well formed up to its end but has no start-of-frame marker, so no size: SOI, a JFIF segment, EOI.
open(path("no-size.jpg"), "wb").write(b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00\xff\xd9")
# A zero width in a PNG header, and a JPEG whose scan data holds bytes that look like a start-of-frame (SOI, JFIF segment,
# start of scan, then FF C0 ...): the sniffer must stop at the start of scan and find no size.
open(path("zero-width.png"), "wb").write(png_header_only(0, 5))
open(path("sos-first.jpg"), "wb").write(
    b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00"
    b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00"
    b"\xff\xc0\x00\x0b\x08\x00\x10\x00\x10\x01\x01\x11\x00\xff\xd9")
