#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Mock-up of the Shapes tool in the real window (feature 0043), made from a screenshot of Easel at 1100 x 720 with Rectangle
chosen (tests/probe/probe-screenshot.c with tool "rectangle"): the toolbox with Shapes in place of the three shape tools, and two
ways of showing the 19 shapes. Run: python3 mockup-panel.py SCREENSHOT.png OUT.png"""
import sys

from PIL import Image, ImageDraw, ImageFont

import shapes as S

WELL_X0, WELL_X1 = 4, 80          # the wells' outer border, from the screenshot
PANEL_END = 640                   # the options column ends here; below is the palette
TINT, ACCENT, INK, BORDER, BG = (246, 205, 181), (182, 34, 0), (51, 51, 55), (224, 224, 225), (250, 250, 251)
NAMES = ["Rectangle", "Rounded Rectangle", "Ellipse"] + S.FULL


def icon(d, name, x, y, size):
    if name == "Rectangle":
        d.rectangle((x, y + 3, x + size, y + size - 3), outline=INK, width=1)
    elif name == "Rounded Rectangle":
        d.rounded_rectangle((x, y + 3, x + size, y + size - 3), radius=3, outline=INK, width=1)
    elif name == "Ellipse":
        d.ellipse((x, y + 2, x + size, y + size - 2), outline=INK, width=1)
    else:
        d.polygon(S.scaled(S.SHAPES[name], x, y, size, size), outline=INK, width=1)


def grid(d, x0, y0, cols, cell, chosen=0):
    rows = (len(NAMES) + cols - 1) // cols
    d.rectangle((x0, y0, x0 + cols * cell + 1, y0 + rows * cell + 1), fill="white", outline=BORDER)
    for i, n in enumerate(NAMES):
        cx, cy = x0 + 1 + (i % cols) * cell, y0 + 1 + (i // cols) * cell
        if i == chosen:
            d.rounded_rectangle((cx, cy, cx + cell - 1, cy + cell - 1), radius=4, fill=TINT, outline=ACCENT)
        icon(d, n, cx + (cell - 16) // 2, cy + (cell - 16) // 2, 16)
    return y0 + rows * cell + 2


def toolbox(src):
    im = src.copy()
    d = ImageDraw.Draw(im)
    d.rectangle((0, 329, 85, PANEL_END), fill=BG)
    im.paste(src.crop((0, 329, 43, 369)), (0, 329))                # Shapes: the chosen button, its icon the chosen shape
    im.paste(src.crop((43, 369, 86, 409)), (43, 329))              # Polygon beside it
    im.paste(src.crop((0, 410, 86, 416)), (0, 372))                # the line under the tools
    return im, d


def variant_a(src):
    im, d = toolbox(src)
    y = grid(d, WELL_X0, 379, 3, 25)
    rest = src.crop((0, 419, 86, PANEL_END))
    im.paste(rest.crop((0, 0, 86, max(0, PANEL_END - (y + 7)))), (0, y + 7))
    d.rectangle((82, 379, 85, 560), fill=(160, 160, 165))           # the column's scrollbar: the rest is below
    return im


def variant_b(src):
    im, d = toolbox(src)
    d.rectangle((WELL_X0, 379, WELL_X1, 409), fill="white", outline=BORDER)
    d.rounded_rectangle((WELL_X0 + 2, 381, WELL_X1 - 2, 407), radius=4, fill=TINT, outline=ACCENT)
    icon(d, "Rectangle", 22, 386, 16)
    d.polygon([(56, 392), (66, 392), (61, 398)], fill=INK)          # opens the popover
    rest = src.crop((0, 419, 86, PANEL_END))
    im.paste(rest.crop((0, 0, 86, PANEL_END - 416)), (0, 416))
    # the popover, open beside the button: 19 cells, five to a row
    x0, y0, cols, cell = 92, 352, 5, 28
    rows = (len(NAMES) + cols - 1) // cols
    d.rounded_rectangle((x0 - 8, y0 - 8, x0 + cols * cell + 9, y0 + rows * cell + 9), radius=10, fill=BG, outline=(200, 200, 205))
    d.polygon([(x0 - 8, 388), (x0 - 15, 394), (x0 - 8, 400)], fill=BG, outline=(200, 200, 205))
    grid(d, x0, y0, cols, cell)
    return im


def main(src_path, out):
    src = Image.open(src_path).convert("RGB")
    a, b = variant_a(src).crop((0, 0, 330, 720)), variant_b(src).crop((0, 0, 330, 720))
    sheet = Image.new("RGB", (330 * 2 + 30, 750), "white")
    d = ImageDraw.Draw(sheet)
    font = ImageFont.load_default(size=14)
    d.text((6, 6), "A. The 19 shapes in the panel", fill="black", font=font)
    d.text((366, 6), "B. One button; the shapes in a popover", fill="black", font=font)
    sheet.paste(a, (0, 30))
    sheet.paste(b, (360, 30))
    sheet.save(out)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
