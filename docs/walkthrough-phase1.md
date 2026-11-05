# Phase 1 walkthrough

One sitting, about half an hour, with the real mouse and keys. One step per journey of `docs/specs/0028-phase1-acceptance.md`. If a step fails, note it and go on.

## Setup (your real settings are never touched)
Close every Easel window first: a second start only hands over to the running one. From the project folder, in one terminal:

```bash
mkdir -p /tmp/easel-walk && python3 - <<'PY'
import struct, zlib
def png(path, w, h, pixel):
    raw = b''.join(b'\0' + b''.join(bytes(pixel(x, y)) for x in range(w)) for y in range(h))
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
# shot.png: white, a blue block at the left, a half-transparent orange block at the top right.
png('/tmp/easel-walk/shot.png', 300, 200, lambda x, y:
    (0, 0, 255, 255) if x < 60 and 80 <= y < 140 else (255, 128, 0, 128) if x >= 200 and y < 60 else (255, 255, 255, 255))
# big.png source: an asymmetric 400 x 300 gradient, for step 7.
png('/tmp/easel-walk/gradient.png', 400, 300, lambda x, y: (x * 255 // 399, y * 255 // 299, 200 if x < y else 40, 255))
# keys.png: a plain white picture, for step 14.
png('/tmp/easel-walk/keys.png', 200, 150, lambda x, y: (255, 255, 255, 255))
PY
# Each start uses a fresh, throwaway settings folder. walk is for most steps; keep is a restart that keeps the folder.
walk() { rm -rf /tmp/easel-walk/config; mkdir -p /tmp/easel-walk/config/easel; [ -n "$SEED" ] && printf "$SEED" > /tmp/easel-walk/config/easel/settings.ini; XDG_CONFIG_HOME=/tmp/easel-walk/config build/src/easel "$@"; }
keep() { XDG_CONFIG_HOME=/tmp/easel-walk/config build/src/easel "$@"; }
```

Everything goes in `/tmp/easel-walk`. Save your pictures there too. Remove it all afterwards with `rm -rf /tmp/easel-walk`. Quit Easel after each step unless the step says otherwise.

## The steps
1. **First picture.** `walk`. Click the Pencil, a swatch, and draw a stroke. Click Line and another swatch, and draw a line. Click Rectangle, choose Outline and fill, and draw one. Ctrl+S, save as `/tmp/easel-walk/first.png`, Ctrl+Q. `keep /tmp/easel-walk/first.png`. **Expect:** the same picture, no `•` in the title, Undo greyed out.
2. **Poster.** `walk`. Image, Attributes: 900 × 600, OK. Right-click a swatch to change the background. Pick the Pencil, then drag the picture's right handle in by 100 pixels. Draw a filled rectangle and a filled ellipse. Text tool: drag a box, type `one `, Ctrl+B, type `bold`, Esc. Ctrl+Z until Undo greys out, then Ctrl+Y until Redo greys out. **Expect:** the first blank picture after the undoing; the finished poster after the redoing, the grown area in its old background color.
3. **Fix a screenshot.** `walk /tmp/easel-walk/shot.png`. Magnifier, click the blue block. Pick Color, click the blue. View, Zoom, 100 %. Fill, click the white. Double-click swatch 1, choose a dark green, OK. Pencil, draw a stroke on the blue. Main menu (☰), Save As, `/tmp/easel-walk/shot.bmp`. Then Ctrl+O and open that BMP. **Expect:** white is now blue, with the green stroke; the orange block looks transparent until the BMP is opened, then is a paler solid orange.
4. **Pixel art.** `walk`. View, Zoom, 800 %. View, Show Grid. Pencil, click four separate pixels. View, Fit to Window, then View, Zoom, 100 %. **Expect:** at 800 % clean squares with grid lines; at Fit to Window and at 100 % the same dots with no grid lines. Save and reopen: no grid line in the file.
5. **Rectangle selection.** `walk`. Draw a red block with Rectangle. With Select, for each of these, mark around part of the block, act, press Enter, then Ctrl+Z: drag it (leaves a hole in the background color); Ctrl and drag (leaves no hole); Shift and drag (a trail, no hole); Shift+Right three times (a short trail, no hole). Then choose Transparent under the tools, mark, drag over a colored area, Enter. **Expect:** the hole shows the checkerboard; the selection's own white pixels still cover what's under them. Ctrl+Z: back to before the lift, no selection. Ctrl+Y: put down again. Save and reopen: the same.
6. **Free-form cut.** `walk`. Draw something colored. Free-Form Select: trace an uneven shape over part of it. Ctrl+X. Ctrl+V. Drag the pasted piece to a clear area. Drag its right handle out to about twice its width, then back past its left edge. Enter. **Expect:** a mirror image of the shape, and only the shape. Save and reopen: the same.
7. **Oversize paste.** `walk /tmp/easel-walk/gradient.png`. Select, Ctrl+A. Right-click, Copy To, save as `/tmp/easel-walk/big.png`. Enter. Attributes: 200 × 150, OK. Main menu (☰), Paste From, `big.png`: a question appears. Fit Picture to Canvas, Enter. **Expect:** size stays 200 × 150, the whole gradient shrunk. Ctrl+Z. Paste From again, Resize Canvas to Picture, Enter. **Expect:** size 400 × 300, the original gradient. Save and reopen: the same.
8. **Rich text.** In a browser, select some text with bold and italic words, and Ctrl+C. `walk`. Text tool: drag a large box. Ctrl+V. Select the last word and Ctrl+U. View, Zoom, 800 %; then 100 %. Esc. **Expect:** the styles kept, the text in the same place after zooming, and put down exactly as it looked. Save and reopen: the same.
9. **Transforms.** `walk`. Draw an uneven stroke. Image: Rotate, Rotate 90°; Flip, Flip Horizontal; Invert Colors; Stretch and Skew with Width 150 %, OK. Select: mark part of it, the same four, Enter. Ctrl+Z five times. **Expect:** exactly the first picture and its size. Save and reopen: the same.
10. **Interrupted tools.** `walk`. For each of Curve (one line drawn), Polygon (two sides drawn) and Gradient (one drag): View, Zoom, 200 %, then 100 %: still waiting. Then, on three separate shapes: Esc (gone, Undo not changed); Enter (put down); choose another tool (put down). **Expect:** six shapes on the picture, none of the three dropped. Save and reopen: the same.
11. **Don't lose work.** `walk /tmp/easel-walk/keys.png`. Draw, Ctrl+Q. Cancel: stays, with the drawing. In a second terminal: `chmod 555 /tmp/easel-walk`. Ctrl+Q, Save. **Expect:** an error naming the file, the drawing still there. Close it. `chmod 755 /tmp/easel-walk`. Ctrl+Q, Save: closes. `keep /tmp/easel-walk/keys.png`: the drawing is there. Draw again, Ctrl+Q, Discard: closes. Open it once more: the second drawing is not there.
12. **Big picture.** `SEED='[history]\nmemory=16\n' walk`. Attributes: 8000 × 8000, OK. View, Zoom, 1600 %. Pencil: click one dot. Select: mark around it, drag it, Enter. View, Zoom, 100 %. Four times, each somewhere new: mark about 1000 × 700, Image, Invert Colors, Enter. Ctrl+Z until Undo greys out. **Expect:** it stops short, with the dot still where you moved it. Save and reopen: the same (it takes a few seconds).
13. **Preferences.** `SEED='# mine\n[view]\ntint=12\n' walk`. Double-click swatch 1, choose a color, OK. Ctrl+, : New Picture: Automatic size off, 300 × 200, Canvas color light blue. Appearance: Workspace color dark blue; Reset Palette. Apply. Tint strength 40, Cancel. Ctrl+Q. `keep`. **Expect:** dark blue workspace, classic colors, a light blue 300 × 200 picture; Tint strength still 12. `cat /tmp/easel-walk/config/easel/settings.ini` still has `# mine` and not `tint=40`. Draw, save and reopen: the same.
14. **Keyboard only.** `walk /tmp/easel-walk/keys.png`. Don't touch the mouse. F6 until a tool button has the focus. Arrows to the Pencil, Space. F6 to the palette, arrows to a swatch, Space. F6 to the canvas. Arrows, Space, arrows, Space: a line. F6 to the tools, arrows to Select, Space. F6 to the canvas. Space, arrows, Space: marks it. Arrows move it. Shift+Down three times: a trail. Enter. Ctrl+S. **Expect:** all of it reachable, and saved with no dialog. Ctrl+O by keyboard, or reopen it: the same.
15. **Another language.** `SEED='[interface]\nlanguage=uk\n' LC_ALL=C walk`. **Expect:** the menus in Ukrainian, under the C locale. Do step 1's drawing, save as `/tmp/easel-walk/uk.png`, quit, `LC_ALL=C keep /tmp/easel-walk/uk.png`. **Expect:** the same picture.
