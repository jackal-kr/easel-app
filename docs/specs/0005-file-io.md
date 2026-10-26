# 0005 File I/O

Status: implemented
Depends on: 0001, 0002, 0004

## Why
So far a picture can only be drawn and saved as PNG, once, and the app always starts blank. People need to open the picture
they want to fix (a screenshot, a photo), start a new one, save under another name or format, and never lose work when they
do any of it.

## Requirements

### Menus and shortcuts
- REQ-0005-01 The header bar shall show an Open button before Save (order per `docs/design.md`: Open, Save, Undo, Redo). The main (hamburger) menu shall hold, in this order and separated from Quit by a line: New (Ctrl+N), Open… (Ctrl+O), Save (Ctrl+S), Save As… (Ctrl+Shift+S), then Quit (Ctrl+Q). The shortcuts work wherever the focus is in the window. (Selection: an Edit submenu between the file items and Quit, REQ-0013-16.) (Pointer note, Clipboard, 2026-10-22: Copy To… and Paste From… after the Edit submenu, REQ-0016-01.)
- REQ-0005-02 New, Open, Save and Save As shall do nothing before the start-up picture exists and while any dialog or prompt is open (the quit prompt, the New or Open prompt, a file, save, replace or error dialog, the color chooser). If one is requested during a pencil stroke, the app shall first end the stroke as if the button were released (REQ-0001-33, REQ-0002-15) and then go on; during a resize drag it shall cancel the drag first (REQ-0004-19). While a New or Open prompt is open, a request to quit (Ctrl+Q, Ctrl+W, the close button) opens no second prompt and closes the open one as Cancel, as for the quit prompt (0001 edge cases), so nothing is lost; while the Open file dialog is open it does nothing; while the dialog of Save or Save As is open, a quit request behaves as REQ-0001-43 says. This differs from the color chooser, which closes as Cancel when the app quits (REQ-0003-14), for two reasons: the color chooser can only change a color, so cancelling it loses nothing, while a prompt or file dialog stands in front of a picture that may be replaced or a file that may be written, and the user's answer decides what is kept; and the Open file dialog is a window of the system that the app cannot rely on closing cleanly. Quitting works again as soon as the prompt or dialog is answered or cancelled. While a picture loads or saves, input waits (the work is done on the spot), so nothing can be requested meanwhile and a quit request is handled after it. (Pointer note, Clipboard, 2026-10-22: a quit request closes the oversize-paste prompt as Cancel, does nothing during Paste From's dialog, acts as for Save As during Copy To's, and waits for a clipboard read, REQ-0016-08, -09, -13, -15.)

### New
- REQ-0005-03 When the user chooses New and the picture is not modified, the app shall replace it at once with a new picture as at start-up: the size and background of the settings file (`auto` resolved against the canvas area at that moment, REQ-0001-02), no file, title "Untitled – Easel", not modified, an empty undo history. The size and background are the values read from the settings file when the app started (a change to the file takes effect at the next start), with `auto` resolved against the canvas area as it is now. The view goes to 100 % zoom, scrolled to the top-left corner. The foreground and background colors, the tool, the grid and the panels stay as they are. If the memory for the new picture cannot be had, the app shows an error dialog with the heading "Could Not Create a New Picture" and the body "There is not enough memory for a picture of W × H pixels." (with the numbers), and keeps the current picture, its file, title, modified state and undo history.
- REQ-0005-04 When the user chooses New and the picture is modified, the app shall first ask as REQ-0001-35 does (heading "Save Changes?", body "“<name>” has unsaved changes.", buttons Cancel, Discard, Save; Save is the default and Esc means Cancel). Save saves as REQ-0001-27..29 and goes on only if the save succeeds; Discard goes on; Cancel or dismissing changes nothing. The same question comes before Open (REQ-0005-08) and, as now, before quitting. Discard only means "go on": nothing is lost until a picture really replaces the current one, so if the file dialog of Open is then cancelled, or the file cannot be opened, the picture stays as it was, still modified. For a picture that has never been saved, the name in the body is "Untitled".

### Open
- REQ-0005-05 When the user chooses Open, the app shall show the system file dialog titled "Open Picture", in the folder of the current picture's file or, if it has none, the Pictures folder, listing the supported pictures (PNG, JPEG, BMP, TIFF, WebP, GIF) by default and every file on request. Cancel changes nothing.
- REQ-0005-06 The app shall open a file by its content, not its name: a PNG named `a.dat` opens, a text file named `a.png` is an error (REQ-0005-09). Pictures are read as 8-bit RGBA: gray, indexed, 16-bit and interlaced variants are converted, a transparent GIF color becomes transparent pixels, JPEG and BMP are fully opaque (a BMP's alpha channel is ignored), and an animated GIF, WebP or PNG opens as its first frame. Two pictures are the same when their sizes and pixels are equal after every fully transparent pixel is taken as #00000000 (0001 AS-27), so a PNG saved by the app and opened again is identical. A picture that carries orientation data (EXIF, as phone photos do) opens upright: it is rotated or mirrored as the data says, and the size the app shows is the upright size (a 4000 × 3000 file tagged "rotate 90°" opens as 3000 × 4000). The app never writes orientation data: a picture saved from it is stored upright and looks the same in any viewer.
- REQ-0005-07 When a picture opens, it shall replace the current one with its exact pixel size (1 to 10000 each way), the file as the picture's file (so Save writes it again, REQ-0001-29), the file's name in the title, not modified, an empty undo history, the status bar showing its size, and the view at 100 % zoom scrolled to the top-left corner.
- REQ-0005-08 When the user chooses Open on a modified picture, the app shall ask first (REQ-0005-04), before showing the file dialog.
- REQ-0005-09 If the file cannot be opened, then the app shall show an error dialog with heading "Could Not Open “<name>”" and one of these bodies: "The file cannot be read." (missing, no permission, a folder, vanished meanwhile), "This is not a picture the app can open." (other content, empty, truncated), "The picture is bigger than 10000 pixels in width or height." or "There is not enough memory for a picture of W × H pixels."; the current picture, its file, title, modified state and undo history stay unchanged.
- REQ-0005-17 If a file's picture is bigger than 10000 pixels in width or in height (after the orientation of REQ-0005-06 is applied), then the app shall refuse it with the error of REQ-0005-09 and body "The picture is bigger than 10000 pixels in width or height.", before the picture is loaded in full and without touching the current picture; a picture of exactly 10000 pixels in either direction opens.

### Save and Save As
- REQ-0005-10 The app shall save PNG (`.png`), JPEG (`.jpg`, `.jpeg`), BMP (`.bmp`), TIFF (`.tif`, `.tiff`) and WebP (`.webp`); the extension of the chosen name, in any case, decides the format. GIF cannot be saved: a name ending `.gif` is an error as for any other unknown extension. (This replaces the "only PNG" rule of REQ-0001-28; a name without an extension still gets `.png`, with the dot and hidden-file rules unchanged.) The error for an unknown extension has the heading "Cannot Save This Kind of File" and the body "Pictures can be saved as PNG, JPEG, BMP, TIFF or WebP.", and nothing is written. So a name such as `report 2024.05.10` is an error too (the text after its last dot is the extension).
- REQ-0005-11 PNG, TIFF and WebP shall keep the picture's transparency. JPEG and BMP cannot, so the app shall save the picture flattened on white: each channel c of a pixel with alpha a (0 to 255) becomes round((c × a + 255 × (255 − a)) / 255); the picture in the window keeps its transparency. PNG, BMP (as flattened), TIFF and WebP saves are lossless: opened again they give exactly the pixels that were saved. JPEG is lossy: in an area of one flat color, a pixel read back differs from the flattened color by at most 8 in any channel. A successful save of any format clears the modified mark (REQ-0001-30), also when the file differs from the picture in the window.
- REQ-0005-12 When the user chooses Save As, the app shall show the file dialog titled "Save Picture As" with the current file's name and folder (`Untitled.png` in the Pictures folder if it has none), save as REQ-0005-10 and REQ-0005-11 describe, and, if it succeeds, make the new file the picture's file, with its name in the title and the picture not modified. If it fails or the dialog is cancelled, the picture's file, title and modified state stay as they were. When the typed name had no extension and `.png` was added, an existing file of that name is replaced only after the question of REQ-0001-42; in every other case (a typed extension, also the current file's own name) the system dialog's own replace confirmation applies and the app asks nothing more.
- REQ-0005-13 When the user chooses Save on a picture that has a file whose extension is one of the saveable ones, the app shall write that file again, in that format, without asking, as REQ-0001-29 says; a file opened as JPEG is therefore written as JPEG. If the file has no saveable extension (a GIF, `a.dat`, no extension), Save behaves as Save As, suggesting the file's name without its extension plus `.png` in its folder. A failed write leaves any existing file with its old content, in every format (REQ-0001-31).
- REQ-0005-14 JPEG shall be saved at the quality set in the settings file, section `[save]`, key `jpeg-quality`, a whole number from 1 to 100 on the usual scale, default 90; a missing, invalid or out-of-range value means 90, silently (syntax as REQ-0001-40; the value is read at start-up). WebP shall be saved lossless. A settings file the app creates also holds `[save]` with `jpeg-quality=90`; an existing file without it is never rewritten (as REQ-0004-15).
- REQ-0005-18 Before it encodes the picture, Save (and Save As) shall check that the memory the encoding needs can be had, and if it cannot, shall show the save error of REQ-0001-31 (heading "Could Not Save “<name>”", the system's reason "Cannot allocate memory") and write nothing, leaving the picture, its file, its title and its modified state as they were. The check is best effort: it cannot foresee that the system runs out of memory later.

### Start-up
- REQ-0005-15 When the app is started with a picture file as an argument (a path, relative to the folder it was started in, or a `file://` address), it shall open that file as the start-up picture instead of a blank one (REQ-0005-06, REQ-0005-07), the window sized as REQ-0001-41. Further arguments are ignored. If it cannot be opened, the app shall show the error of REQ-0005-09 over the window, which then shows the blank picture of the settings. Starting a second copy while one runs is not changed by this feature.

### The saved state
- REQ-0005-16 A failed save shall keep the saved state, as REQ-0001-31 and REQ-0002-15 say; a successful Save As shall move it to the new file; Open and New shall start a new history whose first state is the saved one.

## Scenarios
- AS-1 (REQ-0005-01) Given the window, Then the header bar shows Open before Save, the main menu lists New, Open…, Save, Save As… and Quit with the shortcuts of REQ-0005-01, and Ctrl+N, Ctrl+O and Ctrl+Shift+S are registered | Verified by: automated (ui) + manual M1
- AS-2 (REQ-0005-03) Given an unmodified picture of any size with the grid on and Pencil active, When the user chooses New, Then the picture is the settings' size, filled with the settings' background, titled "Untitled – Easel", Undo is insensitive, the zoom is 100 % at the top-left, the tool, grid and colors are as before; Given settings for a picture whose memory cannot be had (simulated in the test), When the user chooses New, Then the error dialog shows and the current picture, title and Undo are unchanged | Verified by: automated (ui)
- AS-3 (REQ-0005-04) Given a modified picture, When the user chooses New, Then the prompt appears; Discard replaces the picture; Cancel and Esc change nothing; Save with a file saves then replaces, and with a failing save keeps the picture; Given a modified picture, When the user chooses Open, then Discard, then cancels the file dialog or picks a bad file, Then the picture is as it was and still modified | Verified by: automated (ui)
- AS-4 (REQ-0005-05) Given Open, Then a file dialog "Open Picture" shows in the current file's folder (the Pictures folder if none) listing the supported pictures by default and every file on request; Cancel changes nothing | Verified by: manual M2
- AS-5 (REQ-0005-06) Given a PNG with transparent and semi-transparent pixels saved by the app, When it is opened, Then size and every pixel are identical (fully transparent ones as #00000000); a JPEG opens opaque; a BMP (also with an alpha channel, which is ignored), a TIFF, a WebP and a GIF (with its transparent color) open with their size; a gray, an indexed, a 16-bit and an interlaced PNG open as RGBA; an animated GIF and an animated WebP open as their first frame; a PNG named `a.dat` opens and a text file named `a.png` is an error | Verified by: automated (io)
- AS-6 (REQ-0005-07) Given a picture opened from `b.png` in a window showing another picture at 400 % scrolled down, Then the title is "b.png – Easel", the zoom is 100 % at the top-left, the status bar shows its size, the picture is not modified, Undo is insensitive, and Save writes `b.png` without asking | Verified by: automated (ui)
- AS-7 (REQ-0005-08) Given a modified picture, When the user chooses Open, Then the prompt comes before the file dialog | Verified by: automated (ui)
- AS-8 (REQ-0005-09) Given a missing file, an unreadable file, a folder, an empty file, a text file, a picture 10001 pixels wide and (with memory limited in the test) a picture that cannot get its memory, When each is opened, Then the dialog "Could Not Open “<name>”" gives the matching body of REQ-0005-09 and the picture, title, modified state and Undo are unchanged | Verified by: automated (io, the reasons) + automated (ui, the dialog)
- AS-9 (REQ-0005-10) Given names `a.PNG`, `a.jpg`, `a.jpeg`, `a.bmp`, `a.tif`, `a.tiff`, `a.webp`, `a`, `a.`, `.png`, `a.gif`, `a.xyz`, When saved, Then the first seven are written in their format, `a` and `a.` become `a.png`, `.png` becomes `.png.png`, and `a.gif`, `a.xyz` and `report 2024.05.10` show the error of REQ-0005-10 and write nothing | Verified by: automated (io, format by name) + automated (ui, the error)
- AS-10 (REQ-0005-11) Given a picture with transparent pixels, When saved as PNG, TIFF and WebP and opened again, Then the pixels are identical; saved as BMP, Then opened it equals the picture flattened by the formula of REQ-0005-11 and the picture in the window is unchanged; saved as JPEG, Then it is opaque and, in flat areas, within 8 per channel of the flattened picture; a successful save of any of them clears the modified mark | Verified by: automated (io)
- AS-11 (REQ-0005-12) Given a picture with a file `a.png`, When Save As writes `b.jpg`, Then the title is "b.jpg – Easel", not modified, and Save then writes `b.jpg`; When Save As is cancelled or fails, Then file, title and modified state are as before; When the typed name is `b` and `b.png` exists, Then the Replace question of REQ-0001-42 appears; When the typed name is `b.jpg` and it exists, Then the app asks nothing itself (the system dialog's confirmation applies) | Verified by: automated (ui) + manual M2
- AS-12 (REQ-0005-13, REQ-0005-14) Given a picture opened from `b.jpg`, When the user draws and chooses Save, Then `b.jpg` is written as JPEG without a dialog; Given a picture opened from `c.gif`, When the user chooses Save, Then the save dialog shows with the suggestion `c.png` in the same folder | Verified by: automated (ui)
- AS-13 (REQ-0005-15) Given `easel pic.png`, Then the window starts with that picture and its title; Given `easel missing.png`, Then the error appears over the blank picture; Given two files, Then only the first opens; a path and a `file://` address both work (a relative path is made absolute by the system before the app sees it, which is not tested here) | Verified by: automated (ui)
- AS-14 (REQ-0005-16) Given a modified picture saved by Save As and then undone to before the save, Then it is modified; Given a freshly opened picture, Then undo is insensitive and the first edit makes it modified, and undoing that edit makes it unmodified | Verified by: automated (ui)
- AS-15 (REQ-0005-02) Given a dialog or prompt open (including the color chooser) or no picture yet, When New, Open, Save or Save As is requested, Then nothing happens; Given a stroke in progress and a modified picture, When Save is requested, Then the stroke ends and is saved first; When New is requested, Then the stroke ends and the prompt shows; Given a resize drag, When Open is requested, Then the drag is cancelled first; Given the New prompt open, When Ctrl+Q is pressed, Then no second prompt shows, the New prompt closes as Cancel and the picture is as it was; Given the Open file dialog open, When Ctrl+Q is pressed, Then nothing happens; Given the dialog of Save open on a modified picture, When Ctrl+Q is pressed, Then the quit prompt shows, as REQ-0001-43 and 0001 AS-38 say | Verified by: automated (ui)
- AS-16 (REQ-0005-14) Given settings with `jpeg-quality=30`, `100`, `0`, `101`, `abc`, `5.0` or none, Then JPEG saves use 30, 100, 90, 90, 90, 90 and 90, and a smaller file results from 30 than from 100; a created file has `[save]` with `jpeg-quality=90`; an existing file without `[save]` is not rewritten | Verified by: automated (core, the value) + automated (io, the file) + automated (io, quality changes the file size)
- AS-17 (REQ-0005-17) Given pictures of 10000 × 1, 1 × 10000, 10001 × 1, 1 × 10001 and 10001 × 10001 pixels, When each is opened, Then the first two open, the other three show "Could Not Open “<name>”" with the body "The picture is bigger than 10000 pixels in width or height." and the current picture, title, modified state and undo history are unchanged; a file stored as 10001 × 3 and tagged "rotate 90°" opens as 3 × 10001 and is refused too | Verified by: automated (io, the rule) + automated (ui, the dialog)
- AS-18 (REQ-0005-06) Given a JPEG stored as 4 × 2 pixels with a different color in each corner and tagged with each of the eight orientation values, When it is opened, Then the picture has the upright size (2 × 4 for the four rotated tags), and each corner color is where the tag says it belongs; When the picture is saved as PNG and the file is read by a program that ignores orientation data, Then it shows the same upright picture, and the file has no orientation tag; a file without orientation data opens as stored | Verified by: automated (io)
- AS-19 (REQ-0005-18) Given a 6000 x 6000 picture and an address space limited to a little more than the app uses now, When it is saved as PNG, JPEG, BMP, TIFF and WebP, Then each save returns the error and writes no file, the app keeps running, and the picture, its file, its title and its modified state are unchanged | Verified by: automated (io, with the limit, in a subprocess so that a crash is a failed assertion) + automated (ui, the dialog)

## Edge cases
- Opening the file that is already open (even modified): the prompt comes first; Discard re-reads it.
- A file name with spaces, accents or a very long name: shown whole in the title and dialogs.
- A file that changes or vanishes between choosing and reading: the "cannot be read" error.
- Opening a very large picture (10000 x 10000): works if memory allows, else the memory error; it takes as long as it takes (no progress bar in this version).
- Open or New while the color chooser is open: ignored (REQ-0003-14).
- A picture saved in a format that drops transparency stays transparent in the window and modified-state rules are unchanged.
- Save As to the file that is already the picture's file: the replace question appears like any other existing file.
- Zero-byte files, truncated files and files with a picture extension but other content: "This is not a picture the app can open."
- The folder for the dialog is missing or there is no Pictures folder: the user's home folder.
- Save when the picture's folder or file was deleted since: the file is created again if the folder exists, else the save error of REQ-0001-31.
- A read-only target, or a file on a network or removable location that disappears: the save error of REQ-0001-31, old content kept.
- Open a symbolic link: it opens what it points to.

## Out of scope
A question before Save overwrites the file of an opened picture (the owner decided on 2026-10-16 to keep XP Paint's silent save, REQ-0005-13; a lossy JPEG is re-encoded without asking; to revisit, as a setting of Preferences); recent files; drag and drop of files onto the window; opening several files or several windows; the folder choice remembered between runs; colour profiles; animated GIF frames; saving GIF; a quality dialog for JPEG; Print (Window Extras); Paste from / Copy to file (Clipboard); a Preferences entry for the formats (Preferences).

## Open questions
(none)

## Plan

### Approach
One io module reads and writes every picture format through gdk-pixbuf (2.44.5 here, which decodes through glycin's
sandboxed loaders); the core gains the format rules (name to format, flattening, the JPEG quality setting); the window
gains New, Open and Save As around the prompt and save flow it already has for Quit and Save. Loading and saving stay on
the main thread (P5): the window shows the busy cursor and ignores input until the call returns, which REQ-0005-02
already describes.

### How formats are detected, and a missing loader (owner's requirement)
- **Open, by content, with a header sniffer of our own (decided with the owner, 2026-10-15).** The first draft of this
  Plan used `gdk_pixbuf_get_file_info` for the format and the size. Measured on gdk-pixbuf 2.44.5, it is not header-only:
  glycin decodes the whole picture inside the call (a 10001 x 10001 PNG took 113 ms and tried to allocate 300 MB; a
  6000 x 6000 one tried 108 MB), and `GdkPixbufLoader` reports "size-prepared" only after all the data is fed. So io has
  a **sniffer**, `easel_picture_sniff`, that reads only the first bytes of the file (a few hundred; a JPEG's segment
  headers are skipped with `fseek`) and answers the format name and the stored size, or "none":
  - PNG: signature, then the IHDR chunk's width and height; GIF: `GIF87a` or `GIF89a`, then the screen size; BMP: `BM`,
    then the width and height of the DIB header (the 12-byte OS/2 form and the 40-byte and longer forms); JPEG: `FFD8`,
    then the segments up to the first start-of-frame marker (a size marker before the start of scan, or none); TIFF: the
    byte-order mark, then the width and height entries of the first directory; WebP: `RIFF`, `WEBP`, then the size in a
    `VP8 `, `VP8L` or `VP8X` chunk.
  - What the sniffer answers decides three things before any decoding: "none" (empty, truncated before the size, no
    size marker, other content, a format outside the six, a format hidden by the test hook) is "This is not a picture
    the app can open."; `max (w, h) > 10000` is refused (REQ-0005-17), and since turning a picture by 90 degrees only
    swaps its sides the check is the same before and after the orientation; anything else goes on to the memory
    pre-check and the decoder.
  - **What it protects against.** Honest files: a picture that is too big, a file of another kind, a cut-off download.
    It is not a defence against a hostile file. A file whose header says 9000 x 9000 and whose data says otherwise
    passes the sniffer and is decided by the decoder, which runs in glycin's sandbox; the decoder's own errors become
    "not a picture". The app promises no more than that.
  - After the sniffer the file is decoded and converted in one pass by the core (see "Memory when opening"): upright as
    the `orientation` option says (probed: the JPEG loader reports `orientation=6` and does not apply it by itself),
    straight 8-bit RGBA, JPEG and BMP with alpha 255, fully transparent pixels as #00000000.
- **The error bodies.** Not readable (g_file_test or the read fails: missing, permission, a folder) gives "cannot be
  read"; "none" from the sniffer, or a decode error, gives "not a picture"; a size over 10000 gives "bigger than";
  a failed memory pre-check, `GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY` or a failed allocation of the picture gives the
  memory body.
- **Save, by name.** The core maps the extension to a format (REQ-0005-10) and io calls `gdk_pixbuf_save_to_buffer`
  with that format's name, JPEG with `quality`, WebP with `lossless=TRUE`, then the atomic write that PNG uses since
  0001. JPEG and BMP get an RGB pixbuf made from the flattened pixels. Measured on this build (2.44.5 with glycin): the
  WebP saver ignores its options and is always lossless (the same 16580 bytes for every option on a noisy 64 x 64
  picture), so the option is kept for builds whose saver honours it, and what guards REQ-0005-11 is the exact round-trip
  test on noise, not the option.
- **A missing loader or saver, for example WebP.** The six formats are what the dev machine and the Flatpak runtime
  (Packaging) ship, so a missing one is a broken installation; the app still behaves sensibly and adds no strings:
  - the Open dialog's filter lists the six by MIME type whatever is installed; a WebP file then opens with "This is not a
    picture the app can open." (from the app's side it is not one);
  - the Save As dialog has no format list (owner's decision), and the typed-extension rule is about names, not about
    what is installed: `a.webp` is still a WebP name, so it does not get the "Cannot Save This Kind of File" error, whose
    body lists WebP and would contradict itself;
  - the save then fails in the encoder and shows the save error of REQ-0001-31: "Could Not Save “a.webp”" with
    gdk-pixbuf's own reason ("Image type “webp” is not supported", measured); nothing is
    written, an existing file keeps its content, and the picture keeps its file, title and modified state.
  - **How the tests simulate it.** Probed: hiding the loader module (`GDK_PIXBUF_MODULE_FILE` pointing at an empty
    cache) does not remove WebP here, because gdk-pixbuf 2.44 has it built in. So io has one test hook,
    `easel_picture_file_hide_format (const char *name)`, which makes io treat that format as not installed: open
    answers "not a picture", and save hands gdk-pixbuf a format name it does not know, so the error and its message come
    from gdk-pixbuf's real failure path. Tests: io `missing_loader_is_not_a_picture`, io `missing_saver_reports_reason`
    (starting from an existing `a.webp`, which must keep its old bytes), ui `missing_saver_error_dialog`.

### One writer, one formula
- **Writing.** Every format is encoded to a memory buffer by `gdk_pixbuf_save_to_buffer`, then written by the one
  atomic writer that PNG has used since 0001 (`g_file_set_contents_full` with `CONSISTENT`: temporary file, then
  rename). There is no second writer and no encoder writes to the target path itself. The function moves from `png.c`
  to `picture-file.c` unchanged.
- **Flattening.** The formula of REQ-0005-11, `round ((c × a + 255 × (255 − a)) / 255)`, lives in one core function,
  `easel_flatten_on_white`, used for JPEG and BMP alike; core `as10_flatten_formula` checks it at a = 0, 1, 127, 128,
  254, 255 against hand-computed values, and io `as10_round_trips` checks that a saved BMP reads back as exactly that.

### Memory when opening (what it stores, what it copies)
- The decoder's pixbuf holds the pixels at 3 bytes per pixel (JPEG, BMP without alpha) or 4 (the others); the picture
  needs 4. The core converts in one pass from the pixbuf into a newly allocated picture, applying the orientation
  during the same pass, so there is no rotated intermediate copy (`gdk_pixbuf_apply_embedded_orientation` would make
  one) and no third buffer. The pixbuf is freed as soon as the pass ends.
- **Peak while opening:** pixbuf plus picture, about **8 bytes per pixel**: 800 MB for 10000 × 10000 RGBA, 700 MB for a
  10000 × 10000 JPEG, 512 MB for 8000 × 8000 RGBA. Afterwards only the picture stays (4 bytes per pixel). The old
  picture and its undo history are freed only after the new picture exists (REQ-0005-09), so opening on top of a big
  picture adds the old picture to the peak.
- **Not avoided, on purpose:** converting in place inside the pixbuf would need the picture to adopt memory gdk-pixbuf
  owns, which its public API does not hand over, and it cannot work for 3-byte pixels anyway. Freeing the pixbuf right
  after the pass keeps the doubling brief.
- **Full-size copies alive at the peak, and the order they are freed in.** Opening on top of a picture (the usual case):
  three, all the size of the picture being opened or the one being replaced: (1) the old picture, (2) the decoder's
  pixbuf, (3) the new picture. They are freed in this order: the pixbuf as soon as the conversion pass ends (before the
  window touches anything), then the old picture together with its undo history once the canvas shows the new one. So
  the three coexist only during the conversion pass, and afterwards there is one. At start-up or from the command line
  there is no old picture: two. The canvas adds none (its tiles read the picture's own memory, 0004), and the undo
  history of the new picture starts empty.
- **The memory pre-check stays (measured).** glycin is Rust code in our own process, and Rust aborts the process when an
  allocation fails: under a limited address space the decoder's request for 108 MB ended the test program, with no error
  to catch. So before decoding, io reserves what the decoder's pixels and the picture need at most (two buffers of 4
  bytes per pixel, from the size the sniffer found) and lets them go again; if it cannot, the result is the memory error
  and the decoder is never asked. This works for a limited address space (`ulimit -v`, containers) and is how the tests
  force the case; it cannot foresee that the kernel will run out of memory later. The protection is for honest
  files and ordinary limits, not hostile ones.
- **Unknown, measured:** glycin decodes in a separate sandboxed process and passes the pixels through shared memory;
  whether gdk-pixbuf copies them once more is not documented. The benchmark records the peak resident memory
  (VmHWM, reset before each open) next to each time, for 1920 × 1080 and 8000 × 8000 opened alone and on top of a
  picture of the same size, with the expected value (bytes per pixel × pixels × copies) beside it; a measured peak
  above the expected one means a hidden copy, which the Plan then records and the 0005 check decides on.

### "Without freezing": budgets and the busy cursor
`docs/engineering.md` says 8000x8000 "opens and draws without freezing the UI". Agreed with the owner (2026-10-15):
- **Budgets:** from the request (the file is chosen, or `easel_window_open_file` is called) to the first painted frame
  of the opened picture, at most **2 s** for an 8000 × 8000 PNG and **250 ms** for a 1920 × 1080 PNG, both of
  photo-like content, on the dev machine with the default renderer. The benchmark (T006) records the time, the decode
  and conversion times separately and the peak memory, and comes in the same group as the loader.
- **Within budget:** loading is synchronous; the window shows the busy cursor and ignores input until the picture is
  shown (no progress bar, never a half-drawn picture). Afterwards drawing, undo and redo keep the 0004 numbers, which
  the benchmark also records.
- **Over budget:** the window must stay responsive while loading (a worker thread, or a load sliced in idle steps, with
  a spinner and Cancel); P5 has its measured reason then, and an ADR records the choice before it is built.

**The busy cursor shows before decoding starts.** Setting a cursor only asks for it; the compositor shows it when GTK
next sends its requests, and a decode that blocks the main loop right away would block that too. So opening is two
steps: (1) the window sets the busy cursor on itself and on the canvas, blocks input, marks itself as loading and
queues a redraw; (2) the decode runs from a one-shot handler of the frame clock's `after-paint` signal of the next
frame, which comes after GTK has flushed that frame's requests, cursor included. Without a frame clock (a hidden
window) it runs from an idle callback. When the picture is shown, or the open fails, the cursor goes back to the tool's.
Checked by ui `busy_cursor_before_decoding`: right after `easel_window_open_file` returns, the picture is still the old
one, the window says it is loading and its cursor is the busy one; a test hook records the cursor at the moment the
decode starts and it must be the busy one; after the main loop runs, the new picture is there and the cursor is the
tool's again. Manual M3 confirms on screen with the 8000 × 8000 file that the busy cursor appears at once.

### Measurements (T006, 2026-10-15)
Benchmark `perf-0005` (`meson test -C build --benchmark --verbose perf-0005`), the default renderer (GskVulkanRenderer),
AMD Ryzen AI 7 PRO 350 with Radeon 860M, 60 Hz display, window visible, medians of three runs. The picture is a gradient
with noise (a PNG that is not squeezed to nothing). The window's own Open action does not exist yet, so the benchmark does
what it will do, in the same order, on the window's canvas: open the file, give the picture to the canvas, wait for the
first painted frame. T009 repeats it end to end.

| Open a PNG of | Decode | Convert | Request to picture in memory | To first painted frame | Budget |
|---|---|---|---|---|---|
| 1920 x 1080 (8 MB as pixels) | 23 ms | 2 ms | 26 ms | **28 ms** | 250 ms |
| 8000 x 8000 (244 MB as pixels) | 732 ms | 105 ms | 843 ms | **846 ms** | 2000 ms |

| Peak resident memory (VmHWM) | Measured | Baseline of the running app | Measured minus baseline | Expected |
|---|---|---|---|---|
| 1920 x 1080, alone / on top of a same-size picture | 138 / 138 MB | 138 MB | 0 / 0 (below what the baseline hides) | 16 / 24 MB |
| 8000 x 8000, alone | 618 MB | 138 MB | 480 MB | 488 MB (2 copies) |
| 8000 x 8000, on top of a same-size picture | 862 MB | 138 MB | 724 MB | 732 MB (3 copies) |

Reading: both budgets are met by a wide margin (8000 x 8000 opens in 0.85 s, a third of it decoding in glycin's loader
and one eighth converting). The peak is what the Plan says, two copies alone and three on top, so there is **no hidden
third copy**: the decoder's pixels are the only extra. The 138 MB baseline is the window, GTK and the renderer. Memory
the glycin sandbox uses for its own work is in its own process and not in these figures.

End to end (T009, the window's own `easel_window_open_file`, 2026-10-16): the call returns at once (0.0 ms: it only shows
the busy cursor), and the opened picture is on screen after **33 ms** for 1920 x 1080 and **854 ms** for 8000 x 8000,
which is the 28 and 846 ms above plus one painted frame. Both are inside the budgets; the busy cursor is on screen
for the 0.85 s in between.

Decision for "without freezing" (the Plan's rule: about 1 s or less at 8000 x 8000 stays synchronous): **0.85 s is
within it, so loading stays synchronous with the busy cursor; no thread, no ADR.** The 0.85 s freeze is the cost of the
busy cursor design (REQ-0005-02); the owner judges it in M3 on the real 8000 x 8000 file. The numbers for drawing, undo
and redo after an open stay as in 0004 (the picture is the same object), and T009 records them end to end.

### Files
- **New:** `src/core/format.{c,h}` (name to format, replacing `savename`), `src/io/picture-file.{c,h}` (open and save
  every format, replacing `png.{c,h}`), `tests/core/test-format.c`, `tests/io/test-picture-file.c`,
  `tests/data/make-fixtures.py` (Pillow, run by hand) and the small fixture files it writes, `tests/perf/perf-0005.c`.
- **Changed:** `src/core/image.{c,h}` (flatten), `src/core/settings.{c,h}` (`[save] jpeg-quality`, default text),
  `src/ui/window.{c,h,ui}` (Open button, main menu, actions, prompts, dialogs, busy cursor), `src/ui/application.c`
  (accels, files on the command line), both `meson.build` files, `tests/core/test-settings.c`,
  `tests/io/test-settings-file.c`, `tests/ui/test-window.c`, `docs/engineering.md` (io module list).
- **Removed:** `src/core/savename.{c,h}`, `tests/core/test-savename.c`, `src/io/png.{c,h}`: folded into the above, with
  their tests moved, not dropped.

### Key interfaces
- core: `typedef enum { EASEL_FORMAT_PNG, _JPEG, _BMP, _TIFF, _WEBP, EASEL_FORMAT_UNKNOWN } EaselFormat;`
  `EaselFormatResult easel_format_fix_name (const char *path, char **fixed, EaselFormat *format)` (adds `.png` when
  there is no extension, as `savename` did; UNKNOWN for `.gif` and others);
  `bool easel_format_is_flat (EaselFormat f)` (JPEG, BMP);
  `void easel_flatten_on_white (const uint32_t *src, uint8_t *rgb, int n)` (the formula of REQ-0005-11, the only copy);
  `EaselImage *easel_image_from_bytes (const uint8_t *src, int w, int h, int stride, int channels, int orientation)`
  (one pass: orientation 1..8, 3 or 4 channels, alpha-0 to #00000000; NULL on no memory; w, h are the stored size).
- io: `typedef enum { EASEL_OPEN_OK, EASEL_OPEN_UNREADABLE, EASEL_OPEN_NOT_PICTURE, EASEL_OPEN_TOO_BIG,
  EASEL_OPEN_NO_MEMORY } EaselOpenResult;` `EaselOpenResult easel_picture_open (const char *path, EaselImage **img,
  int *w, int *h)` (w, h set for the memory body); `gboolean easel_picture_save (const EaselImage *img, const char *path,
  EaselFormat format, int jpeg_quality, GError **error)` (atomic, as `easel_png_save`).
- ui: `void easel_window_open_file (EaselWindow *self, GFile *file)` (also used by the command line and the tests,
  skipping the file dialog; the prompt still comes first when modified); actions `win.new`, `win.open`, `win.save-as`.
  The quit prompt's code takes a "then" callback, so New, Open and Quit share one prompt (REQ-0005-04).

### New and Open replace the picture
A new `EaselImage` and a new `EaselHistory` (same budget) replace the old ones; the canvas gets `easel_canvas_set_image`,
the zoom is set to 100 % and the scroll to 0, 0; the status bar gets the size. The old picture and history are freed
only after the new picture exists, so a failed open changes nothing (REQ-0005-09). The prompt's Discard sets nothing;
the old picture lives until the replacement succeeds (REQ-0005-04).

### Command line
`application.c` already has `G_APPLICATION_HANDLES_OPEN`. `open` with no window creates it with the first file as its
start-up file: the window loads it where it would create the blank picture (after the canvas area is known), and on
failure creates the blank picture and shows the error. Relative paths and `file://` are resolved by GApplication. With a
window already running, `open` only presents it, as before (REQ-0005-15: a second copy is not changed).

### What grows
Nothing new grows with use. Opening: see "Memory when opening". Saving makes one pixbuf from the picture (4 bytes per
pixel, 3 for JPEG and BMP after flattening) and the encoder's output buffer (the file's size: small for PNG, JPEG and
WebP, up to 4 bytes per pixel for TIFF and BMP), both freed after the write: a peak of up to about 12 bytes per pixel
including the picture, about 770 MB for an 8000 x 8000 TIFF. A failure to get that memory is NOT a save error by itself:
measured (T013, 2026-10-16), gdk-pixbuf's encoders and GLib abort the whole process when an allocation fails ("failed
to allocate 144000000 bytes" for a 6000 x 6000 picture, in all five formats, even with room for two more copies), so the
work would be lost. Hence the check before encoding: three buffers of 4 bytes per pixel (the pixbuf, the encoder's own
copy and its output, which is what the probe showed is needed) are reserved and let go again, the same way Open does it;
if they cannot be had, Save gives the save error of REQ-0001-31 with the system's reason and the picture is kept. Like
Open's, it is best effort: it works for a limited address space and cannot foresee that the kernel runs out of memory
later.

### Settings
| Key | Section | Values | Default | Invalid or out of range |
|---|---|---|---|---|
| `jpeg-quality` | `[save]` | whole number 1..100 | 90 | 90, silently (as 0002 and 0004 keys) |

### Fixtures
`tests/data/make-fixtures.py` (Pillow 12, run by hand; the outputs are committed, a few KB in all): a JPEG of 16 x 8
pixels in four solid 8 x 4 blocks for each of the eight orientation tags (four-pixel blocks, because a JPEG of single
pixels bleeds: probed), a gray, an indexed, a 16-bit and an interlaced PNG, a GIF with a transparent color, a two-frame GIF, a two-frame WebP, a BMP with
an alpha channel, a 10001 x 1, 1 x 10001, 10000 x 1 and 10001 x 10001 PNG (one color, so they stay small), a text file
named `a.png`, an empty file, an SVG. Pillow is used only to make test files, never by the app.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_file_menu` (button order, menu items, accels) + manual M1 |
| 2 | ui `as2_new_picture` |
| 3 | ui `as3_new_and_open_prompt` (incl. Discard then a cancelled or failed open keeps the picture) |
| 4 | manual M2 (the system dialog cannot be driven headless) |
| 5 | io `as5_open_formats` (the fixtures; `a.dat`; text named `a.png`), io `sniff_sizes_match_decoder` |
| 6 | ui `as6_open_replaces_picture` (title, 100 % at top-left checked by rendering the window, status size, Save) |
| 7 | ui `as7_prompt_before_open_dialog` |
| 8 | io `as8_open_errors` (failure path, automated; memory with RLIMIT_AS), ui `as8_open_error_dialog` |
| 9 | core `as9_format_by_name`, ui `as9_unknown_extension_error` |
| 10 | io `as10_round_trips` (PNG, TIFF, WebP exact; BMP = flattened; JPEG within 8 in flat areas) + ui `as10_save_clears_modified` |
| 11 | ui `as11_save_as` + manual M2 (the system's own replace confirmation) |
| 12 | ui `as12_save_keeps_format` |
| 13 | ui `as13_command_line` |
| 14 | ui `as14_saved_state_across_open_and_save_as` |
| 15 | ui `as15_actions_ignored_and_stroke_ended` |
| 16 | core `as16_jpeg_quality_setting`, io `as16_save_section_in_new_file`, io `as16_quality_changes_size` |
| 17 | io `as17_too_big_refused` (failure path, automated; also asserts it returns well under a second, so nothing was decoded), ui `as17_too_big_dialog` |
| 19 | io `as19_save_without_memory` (RLIMIT_AS, in a subprocess), ui `as19_save_memory_error_dialog` |
| 18 | core `as18_orientation_pass` (the eight tags on a 3 x 2 buffer), io `as18_orientation` (all eight tags; the PNG saved from it has no orientation tag and reads upright with orientation ignored) + manual M3 (a real phone photo) |
| - | io `sniff_refuses_broken_files` (truncated header, truncated data, empty file, a header that claims 10000 x 10000 or 40000 x 40000 in a tiny file, a JPEG with no size marker, a text file), io `missing_loader_is_not_a_picture`, io `missing_saver_reports_reason`, ui `missing_saver_error_dialog`, io `unknown_format_is_not_a_picture` (an SVG) |
| - | ui `busy_cursor_before_decoding` + manual M3 |
| - | perf `perf-0005` (T006: open 1920 x 1080 and 8000 x 8000 PNG to first frame, decode and conversion times, peak memory; numbers recorded here) + manual M3 |

A failed save that must keep the old file starts from an existing file (io `as10_failed_save_keeps_old_file`, for
JPEG and TIFF as well as PNG): only then do atomic and in-place writes differ.

### Constitution check
Layering kept (format rules and flattening in core, gdk-pixbuf only in io, dialogs in ui); no threads (P5; a responsive load, if the measurement asks for it, needs an ADR); new key follows P6; tests ship with each task; the spec stays behavior-only.

### Risks
- **glycin's sandbox** may be slow to start or unavailable in some sessions (it uses bwrap); T006 measures the cost, and
  M3 checks a real session. A failure shows up as "not a picture" for every file: the io tests would catch it here.
- **Orientation in other loaders:** probed for JPEG only; the fixtures include the eight tags, so a loader that already
  rotates (and would rotate twice) fails `as18_orientation`.
- **The sniffer disagrees with the decoder** (a format variant whose header it does not read, such as a BigTIFF, or a
  file whose real size differs from its header): the first is "not a picture" and the second is decided by the decoder;
  io `sniff_sizes_match_decoder` checks every fixture, and M3 opens real files from the Pictures folder.
- **Hostile input** is outside what this feature promises (see "How formats are detected"): the decoder is sandboxed by
  glycin, but a file made to exhaust memory inside it aborts the app. A truncated file, an empty file, a header that
  claims a huge size inside a tiny file, and a JPEG with no size marker have tests (io `as8_open_errors`,
  `sniff_refuses_broken_files`).
- **The file dialog** is the system's portal on GNOME; headless tests use `easel_window_open_file` and the existing
  `easel_window_save_to` instead, and the dialog itself is M2.
- **Pillow-made fixtures** could differ from files real programs write; M3 opens real files from the Pictures folder.

## Tasks
Groups, each ending with a look-check by the owner:
A. Formats without the window (T001 to T006): check A is the benchmark numbers against the budgets (the decision point
   for "without freezing"); nothing changes on screen yet.
B. Menus and New (T007, T008): M1.
C. Open (T009, T010): M2 (the Open dialog), M3 (real photos, orientation, the 8000 × 8000 file, the busy cursor).
D. Save and the edge rules (T011, T012): M4 (Save As and the system's replace confirmation).

- [x] T001 Core format names (REQ-0005-10; AS-9) - done when core `as9_format_by_name` passes (every name of AS-9 plus `report 2024.05.10`, case, dot and hidden-file rules), `savename` and its test are folded into `format` with no test dropped, and every 0001 test still passes
- [x] T002 Core flattening and the one-pass conversion with orientation (REQ-0005-06, REQ-0005-11; AS-10, AS-18) - done when core `as10_flatten_formula` (a = 0, 1, 127, 128, 254, 255) and `as18_orientation_pass` (eight tags on a 3 x 2 buffer, 3 and 4 channels, alpha 0 to #00000000) pass
- [x] T003 The `[save] jpeg-quality` setting (REQ-0005-14; AS-16) - done when core `as16_jpeg_quality_setting` and io `as16_save_section_in_new_file` pass and an existing file without `[save]` is not rewritten
- [x] T004 Fixtures, the header sniffer and opening in io (REQ-0005-06, REQ-0005-09, REQ-0005-17; AS-5, AS-8, AS-17, AS-18) - done when `tests/data/make-fixtures.py` and its files are committed and io `as5_open_formats`, `as8_open_errors` (memory with RLIMIT_AS), `as17_too_big_refused` (timed), `as18_orientation`, `missing_loader_is_not_a_picture`, `unknown_format_is_not_a_picture`, `sniff_sizes_match_decoder` and `sniff_refuses_broken_files` pass
- [x] T005 Saving every format in io (REQ-0005-10, REQ-0005-11, REQ-0005-14; AS-10, AS-16) - done when io `as10_round_trips`, `as10_failed_save_keeps_old_file` (from an existing PNG, JPEG and TIFF), `as16_quality_changes_size` and `missing_saver_reports_reason` pass, `png.{c,h}` is folded into `picture-file` with one atomic writer, and the 0001 save tests still pass
- [x] T006 Benchmark opening (budgets of the Plan; REQ-0005-07) - done when `perf-0005` records for 1920 x 1080 and 8000 x 8000 PNG, alone and on top of a same-size picture: time to the first painted frame, decode and conversion times, peak memory with the expected value beside it, and drawing, undo and redo afterwards; the numbers are in the Plan; check A decides (within budget: synchronous; over: stop for an ADR)
- [x] T007 Open button, main menu, actions and their gating (REQ-0005-01, REQ-0005-02; AS-1, AS-15) - done when ui `as1_file_menu` passes (the gating of the actions is tested with New in T008 and completed in T012) and the 0001 to 0004 ui tests still pass
- [x] T008 New and the shared prompt (REQ-0005-03, REQ-0005-04, REQ-0005-16; AS-2, AS-3, AS-14) - done when ui `as2_new_picture`, the New part of `as3_new_and_open_prompt` and the New part of `as14_saved_state_across_open_and_save_as` pass, the quit prompt tests of 0001 still pass with the shared prompt, and manual M1 passes
- [x] T009 Open in the window: busy cursor, replacing the picture, errors, the Open dialog (REQ-0005-04, REQ-0005-05, REQ-0005-07, REQ-0005-08, REQ-0005-09, REQ-0005-16, REQ-0005-17; AS-3, AS-4, AS-6, AS-7, AS-8, AS-14, AS-17) - done when ui `busy_cursor_before_decoding`, `as6_open_replaces_picture` (100 % at the top-left checked by rendering), `as7_prompt_before_open_dialog`, `as8_open_error_dialog`, `as17_too_big_dialog`, the Open parts of `as3_new_and_open_prompt` and `as14_saved_state_across_open_and_save_as` pass, and manual M2 (Open dialog) passes
- [x] T010 A file on the command line (REQ-0005-15; AS-13) - done when ui `as13_command_line` passes (a file, a missing file over the blank picture, two files, a relative path, a `file://` address, a second launch only presenting the window) and manual M3 passes
- [x] T011 Save and Save As in every format (REQ-0005-10, REQ-0005-11, REQ-0005-12, REQ-0005-13, REQ-0005-16; AS-9, AS-10, AS-11, AS-12, AS-14) - done when ui `as9_unknown_extension_error`, `as10_save_clears_modified`, `as11_save_as`, `as12_save_keeps_format`, `missing_saver_error_dialog` and the Save As part of `as14_saved_state_across_open_and_save_as` pass, `docs/engineering.md` lists the new modules, and manual M4 passes
- [x] T012 Strokes, drags and quit around the file actions (REQ-0005-02; AS-15) - done when the rest of ui `as15_actions_ignored_and_stroke_ended` passes (Save and New end a stroke first, Open cancels a resize drag, quit does nothing while the New or Open prompt or a file dialog is open)
- [x] T013 Close the gaps of the first check: Save without memory, two missing tests, two fixtures (REQ-0005-18, REQ-0005-03, REQ-0005-04, REQ-0005-06; AS-3, AS-5, AS-19) - done when io `as19_save_without_memory` passes (5 formats, three limits, in a subprocess), ui `as19_save_memory_error_dialog`, `as2_new_history_records_strokes` (Undo after New undoes only the new strokes) and `as3_cancel_leaves_everything` (Cancel leaves the picture, its history and its unsaved state untouched) pass, io `as5_open_formats` opens the interlaced PNG and the first frame of the animated WebP, `tests/mutants.txt` has a mutant for each new rule and for "a failed save marks the picture saved" and "a failed Save As already changes the file", each killed by an assertion and not by a crash, and `/sdd-check 0005` is repeated

## Check
(second `/sdd-check 0005`, after T013; the first one found the findings below)
- Date and commit: 2026-10-16, `5308217`
- Tests: `meson test` 19 of 19 pass (the ui cases: 97 in all, 27 of them of 0005)
- Mutants: 61 of 61 killed (285 s); none killed only by a crash
- Warnings: 0; lint clean; 18 requirements, 19 scenarios and 13 tasks all traced
- Findings and how each was settled:
  - Save could abort the whole process when memory is short (the Plan had said it was a save error): fixed in T013, REQ-0005-18, io test in a subprocess
  - No test that strokes after New go into the new history, or that Cancel leaves the picture alone: tests added in T013
  - The spec promised interlaced PNG and animated WebP, which had no fixture, and relative paths: fixtures added, relative-path wording narrowed (AS-13)
  - Five mutants killed only by a crash, one stale mutant text: rewritten so that an assertion kills them
  - Saving an opened file is silent (a JPEG is re-encoded without asking): decided by the owner to keep, recorded for Preferences (Out of scope, INDEX)
  - Earlier one-off failures of two ui scroll and zoom cases, unexplained and not reproduced in 22 full runs: left open; the ui tests run one at a time

