#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The contact sheet of Easel's cursors (0029): every drawn cursor as the app draws it, at sizes 24 and 48 and screen
scales 1 and 2, on a light, a dark and a many-colored ground, the hotspot marked by a red ring beside each.

    tools/cursor-sheet.py OUT.png [--build build]

Runs the ui test program's dump case (EASEL_CURSOR_SHEET) into a temporary folder and lays out what it wrote."""
import argparse
import os
import random
import re
import subprocess
import sys
import tempfile

import cairo

COLUMNS = [(24, 1), (24, 2), (48, 1), (48, 2)]
GROUNDS = ['light', 'dark', 'colors']
ORDER = ['pencil', 'airbrush', 'fill', 'pick', 'magnifier', 'crosshair', 'move', 'text',
         'ew-resize', 'ns-resize', 'nwse-resize', 'nesw-resize']


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--build', default='build')
    a = ap.parse_args()
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    with tempfile.TemporaryDirectory() as d:
        env = dict(os.environ, EASEL_CURSOR_SHEET=d)
        r = subprocess.run([os.path.join(root, a.build, 'tests', 'test-window'), '-p', '/0029-sheet'], env=env,
                           capture_output=True, text=True)
        if r.returncode != 0:
            sys.exit('the dump failed:\n' + r.stdout + r.stderr)
        art = {}
        for f in os.listdir(d):
            m = re.match(r'(.+)-(\d+)-x(\d)-(\d+)-(\d+)\.png$', f)
            if m:
                art[(m.group(1), int(m.group(2)), int(m.group(3)))] = (
                    cairo.ImageSurface.create_from_png(os.path.join(d, f)), int(m.group(4)), int(m.group(5)))
        draw(art, a.out)


def draw(art, out):
    cell = 110
    label_w, head_h, row_h = 110, 50, cell
    width = label_w + len(GROUNDS) * len(COLUMNS) * cell + 2 * 10
    height = head_h + len(ORDER) * row_h
    s = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    c = cairo.Context(s)
    c.set_source_rgb(1, 1, 1)
    c.paint()
    c.select_font_face('Cantarell')
    c.set_font_size(12)
    rnd = random.Random(7)
    for g, ground in enumerate(GROUNDS):
        for k, (size, scale) in enumerate(COLUMNS):
            x = label_w + g * (len(COLUMNS) * cell + 10) + k * cell
            c.set_source_rgb(0, 0, 0)
            c.move_to(x + 4, 20)
            c.show_text(ground)
            c.move_to(x + 4, 38)
            c.show_text('%d, scale %d' % (size, scale))
    for i, name in enumerate(ORDER):
        y = head_h + i * row_h
        c.set_source_rgb(0, 0, 0)
        c.move_to(8, y + row_h / 2)
        c.show_text(name)
        for g, ground in enumerate(GROUNDS):
            for k, (size, scale) in enumerate(COLUMNS):
                x = label_w + g * (len(COLUMNS) * cell + 10) + k * cell
                if ground == 'light':
                    c.set_source_rgb(0.97, 0.97, 0.97)
                    c.rectangle(x, y, cell - 4, row_h - 4)
                    c.fill()
                elif ground == 'dark':
                    c.set_source_rgb(0.13, 0.13, 0.13)
                    c.rectangle(x, y, cell - 4, row_h - 4)
                    c.fill()
                else:
                    for yy in range(0, row_h - 4, 6):
                        for xx in range(0, cell - 4, 6):
                            c.set_source_rgb(rnd.random(), rnd.random(), rnd.random())
                            c.rectangle(x + xx, y + yy, min(6, cell - 4 - xx), min(6, row_h - 4 - yy))
                            c.fill()
                img, hx, hy = art[(name, size, scale)]
                ox = x + (cell - 4 - img.get_width()) // 2
                oy = y + (row_h - 4 - img.get_height()) // 2
                c.set_source_surface(img, ox, oy)
                c.paint()
                if ground == 'light':
                    # the hotspot, in device pixels, ringed in red beside the cursor's own pixels
                    c.set_source_rgb(1, 0, 0)
                    c.set_line_width(1)
                    c.arc(ox + hx * scale + scale / 2, oy + hy * scale + scale / 2, 3 + scale, 0, 6.3)
                    c.stroke()
    s.write_to_png(out)
    print('wrote', out)


if __name__ == '__main__':
    main()
