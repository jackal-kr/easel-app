# Phase 2 walkthrough (before the beta)

One sitting, about an hour, with the real mouse and keys: one step per Phase 2 feature (0029 to 0038), then a fractional scale,
a phone photo and the installed packages. If a step fails, note the step's number and what you saw, and go on.

## Setup (your real settings are never touched)
Close every Easel window first. From the project folder, after `ninja -C build`, in one terminal:

```bash
mkdir -p /tmp/easel-walk2 && python3 - <<'PY'
import struct, zlib
def png(path, w, h, pixel):
    raw = b''.join(b'\0' + b''.join(bytes(pixel(x, y)) for x in range(w)) for y in range(h))
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
# subject.png: a red disk and a blue island on a pale sky-to-grass backdrop, a dark band along the bottom edge.
def subject(x, y):
    if (x - 200) ** 2 + (y - 150) ** 2 <= 70 ** 2: return (200, 40, 40, 255)
    if (x - 340) ** 2 + (y - 70) ** 2 <= 20 ** 2: return (40, 60, 200, 255)
    if y >= 270 and 120 <= x < 280: return (40, 40, 50, 255)
    return (150 + y // 6, 190, 230 - y // 4, 255) if y < 200 else (90, 160 - (y - 200) // 3, 70, 255)
png('/tmp/easel-walk2/subject.png', 400, 300, subject)
# big.png: a 3000 x 2000 ramp, for the zoom and Fit to Window steps.
png('/tmp/easel-walk2/big.png', 3000, 2000, lambda x, y: (x * 255 // 2999, y * 255 // 1999, 128, 255))
PY
walk() { rm -rf /tmp/easel-walk2/config; mkdir -p /tmp/easel-walk2/config/easel; XDG_CONFIG_HOME=/tmp/easel-walk2/config build/src/easel "$@"; }
```

Everything goes in `/tmp/easel-walk2`; save there too. Remove it afterwards with `rm -rf /tmp/easel-walk2`.

## Steps
1. **Cursors and icons (0029).** `walk`. Hover the canvas with each tool: every cursor is black with a white outline at your
   desktop's cursor size, Pan the theme's hand; the toolbox icons of the outline tools, Fill and Gradient look like the others.
2. **Options (0030).** Brush: twelve presets in one sunken well, each drawn at real size; hover one: its tooltip ("Circle, 4 pixels")
   and the status bar name it. Airbrush, Eraser (sizes and Opaque/Transparent) and Line's five widths the same way. Drag the
   toolbox's right edge wider and back; double-click it: back to its width.
3. **Below 100 % (0031).** `walk /tmp/easel-walk2/big.png`. View › Zoom › Fit to Window: the ramp is sharp, no blur. Drag a selection
   and a Gradient there: smooth to move, and the moved pixels land where they were shown.
4. **Gradient types (0032).** Gradient: the well shows Linear, Radial and Reflected, each drawn in its cell. Drag a gradient, and while
   it waits choose another type: it changes at once. Enter puts it down; Ctrl+Z takes it back.
5. **Curve bends (0033).** Curve: draw a line; drag a point of it: it bends there and that point stays under the pointer; drag a
   second point; a click away finishes it. Esc during the next curve drops it.
6. **Black and White (0034).** Image › Black and White…: try Threshold, Ordered dither and Error diffusion; OK is one undo step; with
   a selection only the selection changes.
7. **Wheel zoom (0036).** With Pencil, Brush and Select: Ctrl+wheel zooms at the pointer; on a touchpad a pinch zooms; during a
   held drag neither zooms the drawing away. With Ctrl held, a sideways scroll on the touchpad (or a tilted wheel) over the
   picture, over the thumbnail card and over the font card scrolls the view sideways, not zooms.
8. **Remove Background (0035).** `walk /tmp/easel-walk2/subject.png`. Image › Remove Background…: the card opens in the canvas
   area's corner with a Tolerance and a live preview; change the Tolerance with its buttons, the arrows and typing; Cancel leaves the
   picture as it was; OK removes the backdrop in one step (a checker shows through). Save as PNG, open it again: still transparent.
   Fill on the transparent area fills it all, in one piece.
9. **Pieces (0037).** Ctrl+Z, then Remove Background again: click the blue island in the preview: it flips; click again: back.
   F6 to the canvas, arrows to move the marker, Space flips the piece under it.
10. **Brushes (0038).** In the card: Keep this (Alt+K) and paint over the dark band at the bottom: it comes back; Remove this (Alt+R)
    and paint over part of the disk: removed; Ctrl+Z takes back the last stroke or click; with Keep this on, Space held and the
    arrows paint at the marker. Click the island, then paint Keep this over part of it: that part comes back (the later word wins).
11. **A fractional scale.** Settings › Displays › Scale 125 % (turn on fractional scaling if the choice is missing), then steps 1, 2
    and 3 again: the pixels stay sharp squares, the cursor's centre is on the pixel it draws, nothing is blurry. Back to your scale.
12. **A phone photo.** Copy a recent photo from your phone (a JPEG taken in portrait). Open it: upright, not turned on its side; the
    status bar shows its full size. Zoom with Ctrl+wheel, Fit to Window, draw on it, Remove Background on a selection of it, save as
    PNG and as JPEG; both open again as you left them. Never commit the photo.
13. **The packages (0024's M1).** `tools/package.sh install-flatpak`, then start Easel from the app grid: the icon, the name, open and
    save a file in your home folder, Help opens; quit. The same with `tools/package.sh install-deb`, after removing the Flatpak
    (`flatpak uninstall --user` Easel's id), since both show one entry.
14. **The icon (0024 T003).** In the app grid and the dock: say yes, or what to change.
