#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prototype of the extra shapes (feature 0043): each shape is a closed outline in the unit box [0, 1] x [0, 1], x to the
right, y downward, scaled to the box the user drags. Curved parts (heart, callouts, cloud) are sampled into many corners.
Run: python3 docs/reference/proto-0043/shapes.py OUT.png  (draws the mock-up sheet of the three candidate lists)."""
import math
import sys

from PIL import Image, ImageDraw, ImageFont


def regular(n, start=-90.0, r=0.5):
    """A regular n-gon touching the unit box's top, scaled so that it fills the box in both directions."""
    pts = [(math.cos(math.radians(start + 360 * k / n)), math.sin(math.radians(start + 360 * k / n))) for k in range(n)]
    return fit(pts)


def star(n, inner):
    pts = []
    for k in range(2 * n):
        a = math.radians(-90 + 180 * k / n)
        r = 1.0 if k % 2 == 0 else inner
        pts.append((r * math.cos(a), r * math.sin(a)))
    return fit(pts)


def fit(pts):
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    return [((x - x0) / (x1 - x0), (y - y0) / (y1 - y0)) for x, y in pts]


def arrow_right():
    # shaft half as tall as the head, head 40 % of the length
    return [(0, 0.25), (0.6, 0.25), (0.6, 0), (1, 0.5), (0.6, 1), (0.6, 0.75), (0, 0.75)]


def turn(pts, how):
    if how == "left":
        return [(1 - x, y) for x, y in pts]
    if how == "down":
        return [(y, x) for x, y in pts]
    if how == "up":
        return [(y, 1 - x) for x, y in pts]
    return pts


def heart(n=96):
    pts = []
    for k in range(n):
        t = 2 * math.pi * k / n
        x = 16 * math.sin(t) ** 3
        y = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
        pts.append((x, y))
    return fit(pts)


def lightning():
    return fit([(0.38, 0), (0.72, 0), (0.55, 0.36), (0.80, 0.36), (0.25, 1), (0.40, 0.52), (0.18, 0.52)])


def rounded_callout(n=12):
    # body: a rounded rectangle over the top 75 %; tail: from the bottom edge at 25 %..40 % to the corner (0.15, 1)
    r = 0.12
    bx0, by0, bx1, by1 = 0, 0, 1, 0.75
    pts = []

    def arc(cx, cy, a0):
        for k in range(n + 1):
            a = math.radians(a0 + 90 * k / n)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a) * (1 / 0.75) * 0.75))

    arc(bx1 - r, by0 + r, -90)
    arc(bx1 - r, by1 - r, 0)
    pts += [(0.40, by1), (0.15, 1.0), (0.25, by1)]
    arc(bx0 + r, by1 - r, 90)
    arc(bx0 + r, by0 + r, 180)
    return pts


def oval_callout(n=96):
    cx, cy, rx, ry = 0.5, 0.375, 0.5, 0.375
    pts = []
    a_tail0, a_tail1 = math.radians(105), math.radians(125)
    tail_done = False
    for k in range(n):
        a = 2 * math.pi * k / n
        if a_tail0 < a < a_tail1:
            if not tail_done:
                pts.append((0.15, 1.0))
                tail_done = True
            continue
        pts.append((cx + rx * math.cos(a), cy + ry * math.sin(a)))
    return pts


def cloud_callout(n=10):
    # eight bumps on an ellipse, then three small puffs toward the corner
    pts = []
    bumps = 8
    cx, cy, rx, ry = 0.5, 0.36, 0.42, 0.28
    for b in range(bumps):
        a0 = 2 * math.pi * b / bumps
        a1 = 2 * math.pi * (b + 1) / bumps
        mx, my = cx + rx * math.cos((a0 + a1) / 2), cy + ry * math.sin((a0 + a1) / 2)
        p0 = (cx + rx * math.cos(a0), cy + ry * math.sin(a0))
        p1 = (cx + rx * math.cos(a1), cy + ry * math.sin(a1))
        bx, by = mx + (mx - cx) * 0.35, my + (my - cy) * 0.35
        for k in range(n):
            t = k / n
            x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * bx + t * t * p1[0]
            y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * by + t * t * p1[1]
            pts.append((x, y))
    return [(x, y * 0.8) for x, y in fit(pts)]


def circle(cx, cy, r, n=24):
    return [(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


# Shapes made of more than one piece: extra closed outlines drawn with the first (the cloud's tail of two puffs).
EXTRA = {"Cloud Callout": [circle(0.27, 0.86, 0.06), circle(0.15, 0.96, 0.04)]}

SHAPES = {
    "Triangle": [(0.5, 0), (1, 1), (0, 1)],
    "Right Triangle": [(0, 0), (1, 1), (0, 1)],
    "Diamond": [(0.5, 0), (1, 0.5), (0.5, 1), (0, 0.5)],
    "Pentagon": regular(5),
    "Hexagon": regular(6, start=0),
    "Right Arrow": arrow_right(),
    "Left Arrow": turn(arrow_right(), "left"),
    "Up Arrow": turn(arrow_right(), "up"),
    "Down Arrow": turn(arrow_right(), "down"),
    "4-Point Star": star(4, 0.38),
    "5-Point Star": star(5, 0.38),
    "6-Point Star": star(6, 0.5),
    "Rounded Rectangle Callout": rounded_callout(),
    "Oval Callout": oval_callout(),
    "Heart": heart(),
    "Lightning Bolt": lightning(),
    "Cloud Callout": cloud_callout(),
}

FULL = [k for k in SHAPES if k != "Cloud Callout"]
SHORT = ["Triangle", "Right Triangle", "Diamond", "Pentagon", "Hexagon", "Right Arrow", "5-Point Star", "Heart", "Lightning Bolt"]
CLOUD = list(SHAPES)


def scaled(pts, x0, y0, w, h):
    return [(x0 + x * w, y0 + y * h) for x, y in pts]


def sheet(out):
    lists = [("A. Full list, 16 (recommended)", FULL), ("B. Short list, 9", SHORT), ("C. Full list + Cloud callout, 17", CLOUD)]
    big, pad, cols = 72, 14, 9
    font = ImageFont.load_default(size=13)
    small = ImageFont.load_default(size=10)
    rows_h = [30 + ((len(l) + cols - 1) // cols) * (big + 34) for _, l in lists]
    W = pad + cols * (big + pad) + 40 + 3 * 28 + pad + 10
    H = sum(rows_h) + pad * 2
    img = Image.new("RGB", (W, H), "white")
    d = ImageDraw.Draw(img)
    y = pad
    for (title, names), rh in zip(lists, rows_h):
        d.text((pad, y), title, fill="black", font=font)
        for i, name in enumerate(names):
            cx = pad + (i % cols) * (big + pad)
            cy = y + 26 + (i // cols) * (big + 34)
            poly = scaled(SHAPES[name], cx + 2, cy + 2, big - 4, big - 4)
            d.polygon(poly, fill="#ffd84a", outline="#7a3e1d", width=2)
            for extra in EXTRA.get(name, []):
                d.polygon(scaled(extra, cx + 2, cy + 2, big - 4, big - 4), fill="#ffd84a", outline="#7a3e1d", width=2)
            label = name.replace("Rounded Rectangle Callout", "Rect. Callout")
            d.text((cx, cy + big + 2), label, fill="#333", font=small)
        # the options well at real size: 3 columns of 26 x 24 cells, a 16 px icon in each
        wx = pad + cols * (big + pad) + 30
        d.text((wx, y), "panel, 100 %", fill="#666", font=small)
        n = len(names)
        rows = (n + 2) // 3
        d.rectangle((wx - 1, y + 16, wx + 3 * 28, y + 17 + rows * 26), fill="#fafafa", outline="#cfcfcf")
        for i, name in enumerate(names):
            ex = wx + 1 + (i % 3) * 28
            ey = y + 18 + (i // 3) * 26
            if i == 0:
                d.rectangle((ex, ey, ex + 26, ey + 24), fill="#c6dcf5", outline="#3584e4")
            poly = scaled(SHAPES[name], ex + 5, ey + 4, 16, 16)
            d.polygon(poly, fill=None, outline="#222", width=1)
            for extra in EXTRA.get(name, []):
                d.polygon(scaled(extra, ex + 5, ey + 4, 16, 16), fill=None, outline="#222", width=1)
        y += rh
    img.save(out)


if __name__ == "__main__" and (len(sys.argv) < 2 or sys.argv[1] != "--corners"):
    sheet(sys.argv[1] if len(sys.argv) > 1 else "shapes-sheet.png")


def thousandths(t):
    """Nearest thousandth, mirror pairs alike: t and 1 - t always give a and 1000 - a."""
    e = 1e-9                                       # a float just under a half rounds as the half it stands for
    return int(math.floor(t * 1000 + 0.5 + e)) if t <= 0.5 else 1000 - int(math.floor((1 - t) * 1000 + 0.5 + e))


def place(u, w_box, inset_lo, inset_hi):
    """REQ-0043-04: the pixel offset of corner u (thousandths) in a box w_box wide, inset for the width; halves away from the
    middle, so that a mirrored shape is the pixel mirror."""
    n = max(1, w_box - inset_lo - inset_hi)
    lo = inset_lo if w_box - inset_lo - inset_hi >= 1 else (w_box - 1) // 2
    if u <= 500:
        return lo + (u * (n - 1) + 500) // 1000
    return lo + (n - 1) - ((1000 - u) * (n - 1) + 500) // 1000


def corners_md(out):
    """The table the spec cites: each shape's corners in thousandths of the box, in drawing order."""
    with open(out, "w") as f:
        f.write("# Extra shapes: corners (feature 0043)\n\n")
        f.write("Made by `shapes.py --corners`. Each corner is (u, v) in thousandths of the box: u = 0 at its left, 1000 at its right;\n")
        f.write("v = 0 at its top, 1000 at its bottom. Rounded to the nearest thousandth, mirror pairs alike (a value and 1000 less it). A shape with more than one piece\n")
        f.write("lists each piece on its own line.\n\n")
        for name in FULL:
            pieces = [SHAPES[name]] + EXTRA.get(name, [])
            f.write(f"## {name} ({sum(len(p) for p in pieces)} corners)\n")
            for p in pieces:
                q = []
                for x, y in p:
                    a, b = thousandths(x), thousandths(y)
                    if not q or q[-1] != (a, b):
                        q.append((a, b))
                if q[0] == q[-1]:
                    q.pop()
                f.write(" ".join(f"({a}, {b})" for a, b in q) + "\n")
            f.write("\n")


if __name__ == "__main__" and len(sys.argv) > 2 and sys.argv[1] == "--corners":
    corners_md(sys.argv[2])
