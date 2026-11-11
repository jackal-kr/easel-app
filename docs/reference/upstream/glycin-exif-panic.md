# Upstream report draft: glycin's EXIF reader aborts the host process

For JOD to send to glycin's tracker (https://gitlab.gnome.org/GNOME/glycin/-/issues), with `tests/data/exif-damaged.jpg` attached.
Found at the beta gate, 2026-11-10, by fuzzing; Easel's workaround is REQ-0005-06 (AS-20).

---

**Title:** gufo-exif panics on a damaged EXIF block (file.rs:254, `Option::unwrap()` on `None`), aborting the host process via gdk-pixbuf

**Versions:** glycin 2.2.1 (Ubuntu package `glycin-2.2.1+ds`, gufo-exif from `debian/missing-sources`), gdk-pixbuf 2.44.8, GNOME 51.

**What happens:** loading a 704-byte JPEG whose APP1 EXIF block is damaged through gdk-pixbuf aborts the whole calling process:

```
thread '<unnamed>' panicked at .../gufo-exif/src/structure/file.rs:254:77:
called `Option::unwrap()` on a `None` value
fatal runtime error: failed to initiate panic, error 5, aborting
```

The panic happens in the application's own process, not in the sandboxed loader:

```
gly_image_get_transformation_orientation   (libglycin-2.so.0)
load_pixbuf_with_glycin                    gdk-pixbuf/io-glycin-utils.c:467
gdk_pixbuf__glycin_image_stop_load         gdk-pixbuf/io-glycin-utils.c:673
gdk_pixbuf_loader_close                    gdk-pixbuf/gdk-pixbuf-loader.c:871
gdk_pixbuf_new_from_stream / gdk_pixbuf_new_from_file
```

An application that opens user files or clipboard images with gdk-pixbuf therefore crashes, losing unsaved work, on such a file.

**Reproduce:** a program calling `gdk_pixbuf_new_from_file ("exif-damaged.jpg", &error)`, or `gdk_pixbuf_new_from_stream` on its
bytes, with the attached file (both reproduced). Found by fuzzing (libFuzzer, about 7 million inputs derived from small
JPEG/PNG/GIF/BMP/WebP/TIFF files): all 14 crashes were this one site, including EXIF blocks reached past fill bytes (FF FF) before a
marker and past junk bytes between segments; no other panic was found.

**Expected:** a damaged EXIF block is ignored (orientation unknown), or the load fails with a `GError`; never a panic in the caller.

**Attached:** `exif-damaged.jpg` (704 bytes, 32 x 16, a JFIF JPEG with a damaged big-endian EXIF block).

(Easel works around it by removing EXIF blocks from JPEGs before decoding and reading the orientation itself.)
