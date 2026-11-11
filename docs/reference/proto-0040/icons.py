# 0040 prototype (2026-11-11): icon ideas for the Sharp | Smooth cells (0039's and 0040's), each pair at real size (22 x 22 logical
# pixels, as the cell's drawing area) and at 4x, on a light and a dark background. Writes icons.png.
import math, cairo

S = 22

def sharp_line(cr, fg):            # A: today's
    cr.set_antialias(cairo.ANTIALIAS_NONE)
    for k in range(8):
        cr.rectangle(3 + 2 * k, 17 - 2 * k, 2, 2)
    cr.set_source_rgb(*fg); cr.fill()
def smooth_line(cr, fg):
    cr.set_line_width(2); cr.move_to(3.5, 18.5); cr.line_to(18.5, 3.5); cr.set_source_rgb(*fg); cr.stroke()

def pixel_circle(cr, fg, smooth):   # B: a ring
    if smooth:
        cr.set_line_width(2); cr.arc(11, 11, 7, 0, 2 * math.pi); cr.set_source_rgb(*fg); cr.stroke(); return
    cr.set_antialias(cairo.ANTIALIAS_NONE)
    for y in range(22):
        for x in range(22):
            d = math.hypot(x + 0.5 - 11, y + 0.5 - 11)
            if 6 <= d <= 8:
                cr.rectangle(x, y, 1, 1)
    cr.set_source_rgb(*fg); cr.fill()

def wedge(cr, fg, smooth):          # C: a filled slope (the edge of a shape)
    if smooth:
        cr.move_to(3, 19); cr.line_to(19, 19); cr.line_to(19, 5); cr.close_path(); cr.set_source_rgb(*fg); cr.fill(); return
    cr.set_antialias(cairo.ANTIALIAS_NONE)
    for k in range(8):
        cr.rectangle(3 + 2 * k, 19 - 2 * (k + 1) + 2 - 2 * 0, 2 * (8 - k), 2)
    cr.set_source_rgb(*fg); cr.fill()

def big_pixels(cr, fg, smooth):     # D: a magnified 4 x 4 edge, gray steps for Smooth
    shades = [[1, 1, 1, 0.0], [1, 1, 0.0, 0], [1, 0.0, 0, 0], [0.0, 0, 0, 0]]
    soft = [[1, 1, 1, .45], [1, 1, .45, 0], [1, .45, 0, 0], [.45, 0, 0, 0]]
    m = soft if smooth else shades
    cr.set_antialias(cairo.ANTIALIAS_NONE)
    for j in range(4):
        for i in range(4):
            a = m[j][i]
            if a:
                cr.rectangle(3 + 4 * i, 3 + 4 * j, 4, 4); cr.set_source_rgba(*fg, a); cr.fill()

def letter(cr, fg, smooth):         # E: a letter, as text shows it
    cr.set_antialias(cairo.ANTIALIAS_GRAY if smooth else cairo.ANTIALIAS_NONE)
    fo = cairo.FontOptions(); fo.set_antialias(cairo.ANTIALIAS_GRAY if smooth else cairo.ANTIALIAS_NONE)
    cr.set_font_options(fo)
    cr.select_font_face("DejaVu Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD); cr.set_font_size(17)
    cr.move_to(4, 17); cr.text_path("a"); cr.set_source_rgb(*fg); cr.fill()

def curve(cr, fg, smooth):          # F: an S-curve
    if smooth:
        cr.set_line_width(2); cr.move_to(3, 17); cr.curve_to(9, 1, 13, 21, 19, 5); cr.set_source_rgb(*fg); cr.stroke(); return
    cr.set_antialias(cairo.ANTIALIAS_NONE)
    pts = []
    for t in range(200):
        u = t / 199
        x = (1-u)**3*3 + 3*(1-u)**2*u*9 + 3*(1-u)*u*u*13 + u**3*19
        y = (1-u)**3*17 + 3*(1-u)**2*u*1 + 3*(1-u)*u*u*21 + u**3*5
        pts.append((int(x), int(y)))
    for x, y in set(pts):
        cr.rectangle(x, y, 1, 1)
    cr.set_source_rgb(*fg); cr.fill()

IDEAS = [("A  a line (today)", lambda c, f, s: (smooth_line if s else sharp_line)(c, f)),
         ("B  a ring", pixel_circle), ("C  a slope", wedge), ("D  big pixels", big_pixels),
         ("E  a letter", letter), ("F  a curve", curve)]

def cell(fn, smooth, fg, bg, z):
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, S, S); cr = cairo.Context(surf)
    cr.set_source_rgb(*bg); cr.paint(); fn(cr, fg, smooth); return surf

out = cairo.ImageSurface(cairo.FORMAT_ARGB32, 880, 70 + len(IDEAS) * 110)
o = cairo.Context(out); o.set_source_rgb(0.93, 0.93, 0.93); o.paint()
o.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD); o.set_font_size(13); o.set_source_rgb(0.1, 0.1, 0.35)
o.move_to(10, 22); o.show_text("Sharp | Smooth cell ideas: real size (22 px) then 4x; light, then dark")
themes = [((0.13, 0.13, 0.13), (0.98, 0.98, 0.98)), ((0.95, 0.95, 0.95), (0.18, 0.18, 0.2))]
for r, (name, fn) in enumerate(IDEAS):
    y = 50 + r * 110
    o.set_source_rgb(0.1, 0.1, 0.35); o.move_to(10, y + 14); o.show_text(name)
    x = 160
    for fg, bg in themes:
        for smooth in (False, True):
            surf = cell(fn, smooth, fg, bg, 1)
            o.set_source_surface(surf, x, y); o.paint()                        # real size
            o.save(); o.translate(x + 30, y); o.scale(4, 4); o.set_source_surface(surf, 0, 0)
            o.get_source().set_filter(cairo.FILTER_NEAREST); o.paint(); o.restore()
            x += 30 + 4 * S + 12
        x += 10
out.write_to_png("docs/reference/proto-0040/icons.png")
print("ok")
