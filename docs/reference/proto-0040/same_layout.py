# 0040 prototype (2026-11-11): do Sharp (antialias none) and Smooth (gray), both hint full and hint metrics on, lay text out alike?
# Compares every glyph's position, the line breaks and the height of a wrapped paragraph in several fonts and sizes.
import gi, cairo
gi.require_version('Pango', '1.0'); gi.require_version('PangoCairo', '1.0')
from gi.repository import Pango, PangoCairo
TEXT = "The quick brown fox jumps over the lazy dog, 0123456789. Ünïcödé — ffi fl; WAVE Tò Ty."
def lay(aa, font):
    surf = cairo.ImageSurface(cairo.FORMAT_A8, 10, 10); cr = cairo.Context(surf)
    fo = cairo.FontOptions(); fo.set_antialias(aa); fo.set_hint_style(cairo.HINT_STYLE_FULL); fo.set_hint_metrics(cairo.HINT_METRICS_ON)
    layout = PangoCairo.create_layout(cr)
    PangoCairo.context_set_font_options(layout.get_context(), fo)
    PangoCairo.context_set_resolution(layout.get_context(), 96)
    layout.set_font_description(Pango.FontDescription.from_string(font))
    layout.set_width(180 * Pango.SCALE); layout.set_wrap(Pango.WrapMode.WORD_CHAR); layout.set_text(TEXT, -1)
    pos = [tuple(getattr(layout.index_to_pos(i), k) for k in ("x", "y")) for i in range(len(TEXT.encode()))]
    return pos, layout.get_pixel_size()
bad = 0
for f in ("DejaVu Sans", "DejaVu Serif", "Ubuntu Sans", "DejaVu Sans Bold", "DejaVu Serif Italic"):
    for size in (6, 8, 11, 12, 14, 24, 37, 72):
        a, b = lay(cairo.ANTIALIAS_NONE, f"{f} {size}"), lay(cairo.ANTIALIAS_GRAY, f"{f} {size}")
        if a != b:
            bad += 1; print("differs:", f, size, a[1], b[1])
print("cases differing:", bad, "of", 5 * 8)
