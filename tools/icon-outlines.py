#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The eleven toolbox icons of 0029 (option B, smooth 1.5 px lines) as filled outlines: GTK's symbolic-icon loader
ignores strokes, so every line is drawn as its outline here.

    tools/icon-outlines.py src/ui/icons/scalable/actions"""
import math, sys
OUT = sys.argv[1]; W = 1.5; H = W / 2; C = '#222222'
def area(p): return sum(p[i][0]*p[(i+1)%len(p)][1] - p[(i+1)%len(p)][0]*p[i][1] for i in range(len(p))) / 2
def orient(p, solid=True): return p if (area(p) > 0) == solid else p[::-1]
def d(*contours): return ''.join('M' + 'L'.join(f'{x:.2f} {y:.2f}' for x, y in c) + 'Z' for c in contours)
def svg(*paths, comment=''):
    body = ''.join(f'  <path fill="{C}" d="{p}"/>\n' for p in paths)
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">\n'
            f'  <!-- Smooth 1.5 px lines, drawn as filled outlines (REQ-0029-05). -->\n{body}</svg>\n')
def ellipse(cx, cy, rx, ry, n=64): return [(cx + rx*math.cos(2*math.pi*i/n), cy + ry*math.sin(2*math.pi*i/n)) for i in range(n)]
def rrect(x, y, w, h, r, n=10):
    pts = []
    for cx, cy, a0 in ((x+w-r, y+r, -90), (x+w-r, y+h-r, 0), (x+r, y+h-r, 90), (x+r, y+r, 180)):
        for i in range(n+1):
            a = math.radians(a0 + 90*i/n); pts.append((cx + r*math.cos(a), cy + r*math.sin(a)))
    return pts
def ring(outer, inner): return [orient(outer), orient(inner, False)]
def cubic(p0, p1, p2, p3, n=24):
    return [tuple((1-t)**3*a + 3*(1-t)**2*t*b + 3*(1-t)*t**2*c + t**3*e for a, b, c, e in zip(p0, p1, p2, p3))
            for t in (i/n for i in range(n+1))]
def norm(a, b):
    dx, dy = b[0]-a[0], b[1]-a[1]; l = math.hypot(dx, dy) or 1; return (-dy/l, dx/l)
def stroke_open(pts, round_caps=True, n=8):
    L, R = [], []
    for i, p in enumerate(pts):
        a = pts[max(i-1, 0)]; b = pts[min(i+1, len(pts)-1)]; nx, ny = norm(a, b)
        L.append((p[0]+H*nx, p[1]+H*ny)); R.append((p[0]-H*nx, p[1]-H*ny))
    def cap(p, q):   # half circle around p from the left side towards q's direction outwards
        nx, ny = norm(q, p); a0 = math.atan2(ny, nx)
        return [(p[0]+H*math.cos(a0 - math.pi*i/n), p[1]+H*math.sin(a0 - math.pi*i/n)) for i in range(1, n)]
    poly = L + (cap(pts[-1], pts[-2]) if round_caps else []) + R[::-1] + (cap(pts[0], pts[1]) if round_caps else [])
    return orient(poly)
def length_pts(pts):
    s = [0]
    for a, b in zip(pts, pts[1:]): s.append(s[-1] + math.dist(a, b))
    return s
def sub(pts, s, a, b):
    out = []
    for i in range(len(pts)-1):
        for t in (a, b):
            if s[i] <= t <= s[i+1] and (not out or t == b):
                f = (t - s[i]) / (s[i+1]-s[i] or 1); q = (pts[i][0] + f*(pts[i+1][0]-pts[i][0]), pts[i][1] + f*(pts[i+1][1]-pts[i][1]))
                if t == a and not out: out.append(q)
                elif t == b: out.append(q); return out
        if out and a < s[i+1] < b: out.append(pts[i+1])
    return out
def dashes(pts, period_hint=4.2, frac=0.55):
    s = length_pts(pts); L = s[-1]; n = max(3, round(L / period_hint)); per = L / n
    return [stroke_open(sub(pts, s, k*per, k*per + per*frac), round_caps=False) for k in range(n)]
def convex_stroke(poly, n=6):
    poly = orient(poly); k = len(poly); outer = []; inner = []
    for i in range(k):
        p0, p1, p2 = poly[i-1], poly[i], poly[(i+1) % k]
        n1 = norm(p0, p1); n2 = norm(p1, p2)
        # outward normal is -norm for positive area in y-down coordinates; detect by testing centroid
        cx = sum(p[0] for p in poly)/k; cy = sum(p[1] for p in poly)/k
        sgn = -1 if (p1[0]+n1[0]-cx)**2 + (p1[1]+n1[1]-cy)**2 < (p1[0]-n1[0]-cx)**2 + (p1[1]-n1[1]-cy)**2 else 1
        a1 = math.atan2(sgn*n1[1], sgn*n1[0]); a2 = math.atan2(sgn*n2[1], sgn*n2[0])
        while a2 < a1: a2 += 2*math.pi
        if a2 - a1 > math.pi: a2 -= 2*math.pi
        outer += [(p1[0]+H*math.cos(a1+(a2-a1)*j/n), p1[1]+H*math.sin(a1+(a2-a1)*j/n)) for j in range(n+1)]
        # inner: intersection of the two inward-shifted lines
        b = (sgn*n1[0]+sgn*n2[0], sgn*n1[1]+sgn*n2[1]); bl = math.hypot(*b); cosh = bl/2
        inner.append((p1[0]-H*b[0]/bl/cosh, p1[1]-H*b[1]/bl/cosh))
    return [orient(outer), orient(inner, False)]
ic = {}
# Select: a dashed rectangle, the gaps on whole pixels so they stay open at 16 px.
sel = []
for x0, x1 in ((1, 4), (6, 10), (12, 15)):
    sel += [[(x0, 3), (x1, 3), (x1, 4.5), (x0, 4.5)], [(x0, 11.5), (x1, 11.5), (x1, 13), (x0, 13)]]
for y0, y1 in ((4.5, 5), (7, 9), (11, 11.5)):
    sel += [[(1, y0), (2.5, y0), (2.5, y1), (1, y1)], [(13.5, y0), (15, y0), (15, y1), (13.5, y1)]]
ic['select'] = [d(*[orient(p) for p in sel])]
blob = []
B = [((8, 2.5), (11.8, 2.5), (14, 4.6), (13.6, 7.6)), ((13.6, 7.6), (13.2, 10.8), (10.6, 13.4), (7.4, 13.5)),
     ((7.4, 13.5), (4.2, 13.6), (2, 11.4), (2.4, 8.2)), ((2.4, 8.2), (2.8, 4.8), (4.6, 2.5), (8, 2.5))]
for seg in B: blob += cubic(*seg)[:-1]
blob.append(blob[0])
ic['free-select'] = [d(*dashes(blob, 4.0, 0.5))]
ic['magnifier'] = [d(*ring(ellipse(6.5, 6.5, 4.75+H, 4.75+H), ellipse(6.5, 6.5, 4.75-H, 4.75-H)),
                     stroke_open([(9.9, 9.9), (14, 14)]))]
ic['line'] = [d(stroke_open([(2.5, 13.5), (13.5, 2.5)]))]
ic['curve'] = [d(stroke_open(cubic((2, 12), (3.5, 4), (6.5, 3), (8, 8))[:-1] + cubic((8, 8), (9.5, 13), (12.5, 12), (14, 4))))]
ic['rectangle'] = [d(*ring([(1, 3), (15, 3), (15, 13), (1, 13)], [(2.5, 4.5), (13.5, 4.5), (13.5, 11.5), (2.5, 11.5)]))]
ic['rounded-rectangle'] = [d(*ring(rrect(1, 3, 14, 10, 3.5+H), rrect(2.5, 4.5, 11, 7, 3.5-H)))]
ic['ellipse'] = [d(*ring(ellipse(8, 8, 6.25+H, 4.75+H), ellipse(8, 8, 6.25-H, 4.75-H)))]
ic['polygon'] = [d(*convex_stroke([(3, 13), (4.5, 4), (11, 2.5), (14, 9), (9.5, 13.5)]))]
drop = 'M13.5 10C13.5 10 15.3 12 15.3 13.2A1.8 1.8 0 0 1 11.7 13.2C11.7 12 13.5 10 13.5 10Z'
ic['fill'] = [d(*convex_stroke([(7, 1.8), (13, 7.8), (8.3, 12.5), (2.3, 6.5)]), orient([(3.6, 7.5), (12, 7.8), (8.3, 12.2)])), drop]
ic['gradient'] = [d(*ring(rrect(1.5, 1.5, 13, 13, 1.5), [(3, 3), (13, 3), (13, 13), (3, 13)]),
                    orient([(4, 4), (7, 4), (7, 12), (4, 12)]), orient([(8, 4), (10, 4), (10, 12), (8, 12)]),
                    orient([(11, 4), (12, 4), (12, 12), (11, 12)]))]
for k, paths in ic.items():
    open(f'{OUT}/easel-tool-{k}-symbolic.svg', 'w').write(svg(*paths))
