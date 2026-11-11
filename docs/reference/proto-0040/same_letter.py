# 0040 prototype (2026-11-11): with gray smoothing and hint full, are the two "l" of "Hello" (DejaVu Sans 24, 96 dpi, hint metrics on)
# the same pixels? Prints the columns of each "l" and whether they match; and the count of gray pixels.
import gi, cairo
gi.require_version('Pango', '1.0'); gi.require_version('PangoCairo', '1.0')
from gi.repository import Pango, PangoCairo
for font in ("DejaVu Sans 24", "DejaVu Serif 24", "Ubuntu Sans 12"):
    surf = cairo.ImageSurface(cairo.FORMAT_A8, 200, 50)
    cr = cairo.Context(surf)
    fo = cairo.FontOptions(); fo.set_antialias(cairo.ANTIALIAS_GRAY); fo.set_hint_style(cairo.HINT_STYLE_FULL); fo.set_hint_metrics(cairo.HINT_METRICS_ON)
    layout = PangoCairo.create_layout(cr)
    PangoCairo.context_set_font_options(layout.get_context(), fo)
    PangoCairo.context_set_resolution(layout.get_context(), 96)
    layout.set_font_description(Pango.FontDescription.from_string(font))
    layout.set_text("Hello", -1)
    PangoCairo.show_layout(cr, layout)
    surf.flush()
    data, stride = surf.get_data(), surf.get_stride()
    it = layout.get_iter(); boxes = []
    for i in range(5):
        r = layout.index_to_pos(i); boxes.append((r.x // Pango.SCALE, (r.x + r.width) // Pango.SCALE))
    def cols(b):
        return [bytes(data[y * stride + b[0]: y * stride + b[1]]) for y in range(50)]
    gray = sum(1 for v in bytes(data) if 0 < v < 255)
    print(font, "l boxes", boxes[2], boxes[3], "same:", cols(boxes[2]) == cols(boxes[3]), "gray pixels:", gray)
