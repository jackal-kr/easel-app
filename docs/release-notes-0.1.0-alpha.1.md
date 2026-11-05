# Easel 0.1.0-alpha.1 (draft of the release notes)

Draft: these go on the GitHub release, after M1. Lines marked TODO wait for M1.

Easel is a simple drawing program for GNOME, in the style of Paint in Windows XP. **This is an early alpha:** expect rough edges, and keep copies of pictures you care about.

## Install one of them
Installing both shows one entry in the app grid, so please choose one.

**Flatpak, for any distribution.** Needs Flatpak and Flathub, for the GNOME 51 runtime it downloads:

    sudo apt install flatpak
    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    flatpak install --user easel-0.1.0-alpha.1.flatpak

Run it from the app grid, or `flatpak run io.github.jackal_kr.Easel`. To open a file from a terminal, use `flatpak run --file-forwarding io.github.jackal_kr.Easel @@ picture.png @@`; a plain path cannot be seen from inside the sandbox.

**Deb, for Ubuntu 26.10 on x86-64:**

    sudo apt install ./easel_0.1.0-alpha.1_amd64.deb

## Remove
- Flatpak: `flatpak uninstall --user io.github.jackal_kr.Easel`. Adding `--delete-data` also removes your settings. `flatpak uninstall --user --unused` removes the runtime if nothing else uses it.
- Deb: `sudo apt remove easel`.

Your pictures are never touched. Settings and your palette stay in `~/.config/easel` (deb) or `~/.var/app/io.github.jackal_kr.Easel/config/easel` (Flatpak). The two do not share them; copy the folder across to keep them.

## Check the download
`sha256sum -c SHA256SUMS`. This catches a damaged download, not a tampered one: the files are not signed. Built from commit TODO.

## Known limits
- Tested on Ubuntu 26.10 with GNOME only.
- The hand walkthrough of the whole app, a check with phone photos, and fractional display scales (125 %, 150 %) are not done yet.
- Translations (cs, de, es, fr, hu, it, pl, pt, ro, ru, uk) are unreviewed machine-written drafts.
- With Easel already open, opening a file from the file manager brings the window to the front and does not open that file. Use Open.
- Flatpak: the Picture tab in Print: TODO from M1. Help: TODO from M1.

## How this was made
AI (Claude) wrote the code, the tests, the translations and the icon, working from specs that JOD wrote and approved. JOD has tried the features by hand as each was built; he has not reviewed the code line by line, nor read the translations.

## Report a problem
https://github.com/jackal-kr/easel-app/issues
