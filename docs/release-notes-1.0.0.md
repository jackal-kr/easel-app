# Easel 1.0.0

Easel is a simple drawing program for GNOME, in the style of Paint in Windows XP. This is the first full release: everything from Paint that Easel set out to do is in, with the extras below, and it has been tested by the owner's daily use since the beta.

## New since 0.1.0-beta.1
- **Shapes:** one Shapes tool replaces Rectangle, Rounded Rectangle and Ellipse, with 19 shapes to choose from: triangles, a diamond, a pentagon, a hexagon, four arrows, three stars, two callouts, a heart and a lightning bolt. A shape you draw waits with handles: move it, resize it, mirror it by dragging a handle past the other side, or change its color, width and fill, until Enter or a click away puts it down. Esc removes it.
- **Crop to Selection** (Image menu and the selection's menu, Ctrl+Shift+K): the picture becomes the selection's rectangle.
- **Invert Selection** (Edit menu and the selection's menu, Ctrl+Shift+J): everything outside the selection becomes the selection.
- **Ctrl+Shift+arrow** now moves a copy of the selection 10 pixels and leaves the original in place, with no blank strip behind it.

## Install one of them
Installing both shows one entry in the app grid, so please choose one.

**Flatpak, for any distribution.** Needs Flatpak and Flathub, for the GNOME 51 runtime it downloads:

    sudo apt install flatpak
    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    flatpak install --user easel-1.0.0.flatpak

Run it from the app grid, or `flatpak run io.github.jackal_kr.Easel`. To open a file from a terminal, use `flatpak run --file-forwarding io.github.jackal_kr.Easel @@ picture.png @@`; a plain path cannot be seen from inside the sandbox.

**Deb, for Ubuntu 26.10 on x86-64:**

    sudo apt install ./easel_1.0.0_amd64.deb

Coming from the beta: install the new file over it; your settings and palette are kept.

## Remove
- Flatpak: `flatpak uninstall --user io.github.jackal_kr.Easel`. Adding `--delete-data` also removes your settings. `flatpak uninstall --user --unused` removes the runtime if nothing else uses it.
- Deb: `sudo apt remove easel`.

Your pictures are never touched. Settings and your palette stay in `~/.config/easel` (deb) or `~/.var/app/io.github.jackal_kr.Easel/config/easel` (Flatpak). The two do not share them; copy the folder across to keep them.

## Check the download
`sha256sum -c SHA256SUMS`. This catches a damaged download, not a tampered one: the files are not signed. Built from the commit tagged `v1.0.0`.

## Known limits
- Tested on Ubuntu 26.10 with GNOME only.
- Translations (cs, de, es, fr, hu, it, pl, pt, ro, ru, uk) are unreviewed machine-written drafts.
- With Easel already open, opening a file from the file manager brings the window to the front and does not open that file. Use Open.
- Remove Background does not recognise subjects: a pale subject on a pale background, soft edges and line drawings need a lower Tolerance or the Keep this and Remove this brushes.
- A waiting shape can be moved with the keyboard but not resized.

## How this was made
AI (Claude) wrote the code, the tests, the translations and the icon, working from specs that JOD wrote and approved. JOD has tried the features by hand as each was built; he has not reviewed the code line by line, nor read the translations.

## Report a problem
https://github.com/jackal-kr/easel-app/issues
