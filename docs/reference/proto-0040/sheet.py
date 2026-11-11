# 0040 prototype (2026-11-11): text as the app draws it (96 dpi, hint metrics on, whole-pixel positions), Sharp (antialias none,
# hint full) and Smooth (gray antialias) with hint full and hint slight, black on white, at 100 % and at 800 %. Writes sheet.png.
import gi, cairo
gi.require_version('Pango', '1.0'); gi.require_version('PangoCairo', '1.0')
from gi.repository import Pango, PangoCairo

TEXT = "Hello, Easel 12"
VARIANTS = [("Sharp (today)", cairo.ANTIALIAS_NONE, cairo.HINT_STYLE_FULL),
            ("Smooth, hint full", cairo.ANTIALIAS_GRAY, cairo.HINT_STYLE_FULL),
            ("Smooth, hint slight", cairo.ANTIALIAS_GRAY, cairo.HINT_STYLE_SLIGHT)]

def render(aa, hint, font="Ubuntu Sans 12"):
    w, h = 150, 22
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
    cr = cairo.Context(surf)
    cr.set_source_rgb(1, 1, 1); cr.paint()
    fo = cairo.FontOptions(); fo.set_antialias(aa); fo.set_hint_style(hint); fo.set_hint_metrics(cairo.HINT_METRICS_ON)
    layout = PangoCairo.create_layout(cr)
    PangoCairo.context_set_font_options(layout.get_context(), fo)
    PangoCairo.context_set_resolution(layout.get_context(), 96)
    layout.set_font_description(Pango.FontDescription.from_string(font))
    layout.set_text(TEXT, -1)
    cr.set_source_rgb(0, 0, 0); cr.move_to(2, 2)
    PangoCairo.show_layout(cr, layout)
    return surf

def blit(dst, src, x, y, z):
    cr = cairo.Context(dst)
    cr.translate(x, y); cr.scale(z, z)
    cr.set_source_surface(src, 0, 0); cr.get_source().set_filter(cairo.FILTER_NEAREST); cr.paint()

def label(dst, text, x, y):
    cr = cairo.Context(dst)
    layout = PangoCairo.create_layout(cr)
    layout.set_font_description(Pango.FontDescription.from_string("Sans Bold 11"))
    layout.set_text(text, -1); cr.set_source_rgb(0.1, 0.1, 0.4); cr.move_to(x, y); PangoCairo.show_layout(cr, layout)

out = cairo.ImageSurface(cairo.FORMAT_ARGB32, 1260, 3 * 230 + 20)
c = cairo.Context(out); c.set_source_rgb(0.85, 0.85, 0.85); c.paint()
for i, (name, aa, hint) in enumerate(VARIANTS):
    s = render(aa, hint)
    y0 = 10 + i * 230
    label(out, name + "  (100 %, then 800 % of the first letters)", 10, y0)
    blit(out, s, 10, y0 + 22, 1)
    crop = cairo.ImageSurface(cairo.FORMAT_ARGB32, 70, 22)
    cc = cairo.Context(crop); cc.set_source_surface(s, 0, 0); cc.paint()
    blit(out, crop, 170, y0 + 22, 8)
out.write_to_png("sheet.png")
print("wrote sheet.png")
