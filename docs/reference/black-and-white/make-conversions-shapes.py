#!/usr/bin/env python3
# 0034 reference (2026-11-11): builds make-conversions-shapes.c against the core of build/ and lays its four pictures out as
# conversions-shapes.png: each with a 4x strip of the ramp's dark end (the magenta box) under it. Run from the project root after a build.
import os, subprocess, tempfile, cairo

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(os.path.dirname(os.path.dirname(here)))
with tempfile.TemporaryDirectory() as tmp:
    exe = os.path.join(tmp, 'make')
    subprocess.run(['cc', '-O2', '-I', os.path.join(root, 'src/core'), os.path.join(here, 'make-conversions-shapes.c'),
                    os.path.join(root, 'build/src/libeasel-core.a'), '-lm', '-o', exe], check=True)
    subprocess.run([exe, tmp], check=True)
    names = [('original', 'Original'), ('threshold', 'Threshold'), ('ordered', 'Ordered dither'), ('diffusion', 'Error diffusion')]
    out = cairo.ImageSurface(cairo.FORMAT_RGB24, 1340, 330)
    cr = cairo.Context(out); cr.set_source_rgb(0.92, 0.92, 0.92); cr.paint()
    for k, (name, label) in enumerate(names):
        data = open(os.path.join(tmp, name + '.ppm'), 'rb').read().split(b'\n', 3)[3]
        pic = cairo.ImageSurface(cairo.FORMAT_RGB24, 320, 200)
        buf = pic.get_data(); stride = pic.get_stride()
        for y in range(200):
            for x in range(320):
                r, g, b = data[3 * (y * 320 + x): 3 * (y * 320 + x) + 3]
                buf[y * stride + 4 * x: y * stride + 4 * x + 4] = bytes((b, g, r, 255))
        pic.mark_dirty()
        x0 = 12 + k * 332
        cr.set_source_rgb(0, 0, 0); cr.select_font_face('Sans'); cr.set_font_size(14); cr.move_to(x0, 20); cr.show_text(label)
        cr.set_source_surface(pic, x0, 30); cr.paint()
        cr.save(); cr.translate(x0, 234); cr.scale(4, 4); cr.set_source_surface(pic, 0, -10)
        cr.get_source().set_filter(cairo.FILTER_NEAREST); cr.rectangle(0, 0, 80, 20); cr.fill(); cr.restore()
        cr.set_source_rgb(1, 0, 0.5); cr.set_line_width(1); cr.rectangle(x0 + 0.5, 40.5, 79, 19); cr.stroke()
    out.write_to_png(os.path.join(here, 'conversions-shapes.png'))
    print('wrote', os.path.join(here, 'conversions-shapes.png'))
