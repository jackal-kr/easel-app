#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The 16 shape outlines of 0043 (REQ-0043-16): each shape's polygon from docs/reference/proto-0043/corners.md, placed in a 16 x 16
box in the proportions of lists-sheet.png, drawn as a smooth 1.5 px line in the weight of REQ-0029-05: GTK's symbolic-icon loader
ignores strokes, so the line is written as its outline, round on the outside of every turn, mitred on the inside.

    tools/shape-icons.py docs/reference/proto-0043/corners.md src/ui/icons/scalable/actions"""
import math, re, sys

CORNERS, OUT = sys.argv[1], sys.argv[2]
H = 0.75                     # half the 1.5 px line
COLOR = '#222222'
NAMES = ['triangle', 'right-triangle', 'diamond', 'pentagon', 'hexagon', 'right-arrow', 'left-arrow', 'up-arrow', 'down-arrow',
         '4-point-star', '5-point-star', '6-point-star', 'rounded-rectangle-callout', 'oval-callout', 'heart', 'lightning-bolt']
# The line's middle runs in this box: the full 12.5 px between 1.75 and 14.25, as the tool icons' outer edges reach 1 and 15; the
# arrows are less deep across their shaft than along it, as on the sheet.
FULL = (1.75, 1.75, 12.5, 12.5)
WIDE = (1.75, 3.625, 12.5, 8.75)
TALL = (3.625, 1.75, 8.75, 12.5)
BOX = {'right-arrow': WIDE, 'left-arrow': WIDE, 'up-arrow': TALL, 'down-arrow': TALL}


def read_corners(path):
    text = open(path).read()
    sections = re.findall(r'^## (.+?) \((\d+) corners\)\n(.*?)(?=^## |\Z)', text, re.M | re.S)
    assert len(sections) == 16
    out = []
    for name, n, body in sections:
        pts = [(int(u), int(v)) for u, v in re.findall(r'\((\d+), (\d+)\)', body)]
        assert len(pts) == int(n), name
        out.append(pts)
    return out


def area(p):
    return sum(p[i][0] * p[(i + 1) % len(p)][1] - p[(i + 1) % len(p)][0] * p[i][1] for i in range(len(p))) / 2


def clean(p):
    """Consecutive corners closer than 0.05 px become one (the Heart repeats corners at its tip and its notch)."""
    out = []
    for q in p:
        if not out or math.dist(out[-1], q) > 0.05:
            out.append(q)
    if math.dist(out[0], out[-1]) <= 0.05:
        out.pop()
    return out


def outline(poly, arc_steps=6):
    """The outer and the inner edge of the line along the closed polygon: on the outside of a turn a round join, on its inside the
    meeting point of the two shifted sides."""
    poly = clean(poly)
    if area(poly) < 0:
        poly = poly[::-1]
    k = len(poly)
    outer, inner = [], []
    for i in range(k):
        p0, p1, p2 = poly[i - 1], poly[i], poly[(i + 1) % k]
        t1 = ((p1[0] - p0[0]) / math.dist(p0, p1), (p1[1] - p0[1]) / math.dist(p0, p1))
        t2 = ((p2[0] - p1[0]) / math.dist(p1, p2), (p2[1] - p1[1]) / math.dist(p1, p2))
        n1, n2 = (t1[1], -t1[0]), (t2[1], -t2[0])          # outward normals for a positive area
        cross = t1[0] * t2[1] - t1[1] * t2[0]
        dot = n1[0] * n2[0] + n1[1] * n2[1]
        miter = ((n1[0] + n2[0]) / (1 + dot), (n1[1] + n2[1]) / (1 + dot)) if dot > -0.999 else n1

        def arc(sign):
            a1, a2 = math.atan2(sign * n1[1], sign * n1[0]), math.atan2(sign * n2[1], sign * n2[0])
            while a2 - a1 > math.pi:
                a2 -= 2 * math.pi
            while a1 - a2 > math.pi:
                a2 += 2 * math.pi
            return [(p1[0] + H * math.cos(a1 + (a2 - a1) * j / arc_steps), p1[1] + H * math.sin(a1 + (a2 - a1) * j / arc_steps))
                    for j in range(arc_steps + 1)]
        if cross >= 0:            # the polygon turns outward here: round outside, mitred inside
            outer += arc(1)
            m = (p1[0] - H * miter[0], p1[1] - H * miter[1])
            theta = math.acos(max(-1.0, min(1.0, -(t1[0] * t2[0] + t1[1] * t2[1]))))     # the corner's angle
            r = 1 / (2 * math.tan(theta / 2)) if theta > 0.01 else 0                    # where the hole is one pixel wide
            if r > 0.5:
                # A sharp corner: the hole stops where it is one pixel wide, so that no sliver of it is left half covered.
                along = r / math.cos(theta / 2)
                inner += [(m[0] - along * t1[0], m[1] - along * t1[1]), (m[0] + along * t2[0], m[1] + along * t2[1])]
            else:
                inner.append(m)
        else:                     # a notch: mitred outside (bevelled where the mitre would reach past twice the half line), round inside
            if math.hypot(*miter) <= 2:
                outer.append((p1[0] + H * miter[0], p1[1] + H * miter[1]))
            else:
                outer += [(p1[0] + H * n1[0], p1[1] + H * n1[1]), (p1[0] + H * n2[0], p1[1] + H * n2[1])]
            inner += arc(-1)
    return outer, inner[::-1]


def path(*contours):
    return ''.join('M' + 'L'.join(f'{x:.2f} {y:.2f}' for x, y in c) + 'Z' for c in contours)


def svg(d):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">\n'
            '  <!-- A shape of Shapes (0043, REQ-0043-16): its corners of corners.md as a smooth 1.5 px line, drawn as filled outlines. -->\n'
            f'  <path fill="{COLOR}" d="{d}"/>\n</svg>\n')


for name, uv in zip(NAMES, read_corners(CORNERS)):
    bx, by, bw, bh = BOX.get(name, FULL)
    poly = [(bx + u / 1000 * bw, by + v / 1000 * bh) for u, v in uv]
    outer, inner = outline(poly)
    open(f'{OUT}/easel-shape-{name}-symbolic.svg', 'w').write(svg(path(outer, inner)))
