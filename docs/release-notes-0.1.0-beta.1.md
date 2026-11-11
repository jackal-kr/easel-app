# Easel 0.1.0-beta.1

Easel is a simple drawing program for GNOME, in the style of Paint in Windows XP. **This is a beta:** every feature planned for the first release is in, and it has been tested by the owner's daily use; expect a few rough edges still, and keep copies of pictures you care about.

## New since 0.1.0-alpha.1
- **Remove Background** (Image menu): makes the background of the picture, or of the selection, transparent. It finds the background by its colors and edges, with a Tolerance and a live preview. Click a piece of the preview to keep or remove it, or paint over parts with **Keep this** and **Remove this**; Ctrl+Z takes back the last click or stroke.
- **Black and White** (Image menu): Threshold, Ordered dither or Error diffusion, for the picture or the selection.
- **Gradient types:** Linear, Radial and Reflected.
- **Curve:** bend the line by dragging a point of it, up to two bends; a click away finishes it.
- **Smooth shapes and brushes:** Line, Curve, the shapes and the Brush can draw with smooth edges (Sharp or Smooth under their options). Off by default; Preferences sets what they start with.
- **Zoom with every tool:** Ctrl+wheel and the touchpad pinch zoom at the pointer, whatever tool is chosen.
- **Clearer tools:** cursors drawn in one style at your desktop's cursor size, and the Brush, Airbrush and Eraser options drawn at their real size. The toolbox can be made wider by dragging its edge.
- **Below 100 %** the picture stays sharp: no blur when zoomed out.
- **Fixes:** a damaged photo's EXIF block can no longer crash the app on opening; three spots of undefined behaviour found by sanitizers are gone; a size chosen in an empty text box is no longer lost when you click into the box.

## Install one of them
Installing both shows one entry in the app grid, so please choose one.

**Flatpak, for any distribution.** Needs Flatpak and Flathub, for the GNOME 51 runtime it downloads:

    sudo apt install flatpak
    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    flatpak install --user easel-0.1.0-beta.1.flatpak

Run it from the app grid, or `flatpak run io.github.jackal_kr.Easel`. To open a file from a terminal, use `flatpak run --file-forwarding io.github.jackal_kr.Easel @@ picture.png @@`; a plain path cannot be seen from inside the sandbox.

**Deb, for Ubuntu 26.10 on x86-64:**

    sudo apt install ./easel_0.1.0-beta.1_amd64.deb

Coming from the alpha: install the new file over it; your settings and palette are kept.

## Remove
- Flatpak: `flatpak uninstall --user io.github.jackal_kr.Easel`. Adding `--delete-data` also removes your settings. `flatpak uninstall --user --unused` removes the runtime if nothing else uses it.
- Deb: `sudo apt remove easel`.

Your pictures are never touched. Settings and your palette stay in `~/.config/easel` (deb) or `~/.var/app/io.github.jackal_kr.Easel/config/easel` (Flatpak). The two do not share them; copy the folder across to keep them.

## Check the download
`sha256sum -c SHA256SUMS`. This catches a damaged download, not a tampered one: the files are not signed. Built from the commit tagged `v0.1.0-beta.1`.

## Known limits
- Tested on Ubuntu 26.10 with GNOME only.
- Translations (cs, de, es, fr, hu, it, pl, pt, ro, ru, uk) are unreviewed machine-written drafts.
- With Easel already open, opening a file from the file manager brings the window to the front and does not open that file. Use Open.
- Remove Background does not recognise subjects: a pale subject on a pale background, soft edges and line drawings need a lower Tolerance or the Keep this and Remove this brushes.

## How this was made
AI (Claude) wrote the code, the tests, the translations and the icon, working from specs that JOD wrote and approved. JOD has tried the features by hand as each was built; he has not reviewed the code line by line, nor read the translations.

## Report a problem
https://github.com/jackal-kr/easel-app/issues
