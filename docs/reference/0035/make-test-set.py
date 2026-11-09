#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Builds 0035's test set (AS-2): subjects cut from openly licensed photos (LICENSES.md) laid on plain or gently varying
backgrounds, so that each picture's true subject mask is known exactly.

    python3 make-test-set.py CACHE_DIR OUT_DIR      (needs Pillow and NumPy; downloads the cut-outs into CACHE_DIR once)

Each picture NN.png (960 x 720, opaque) has NN-mask.png beside it: white where the subject is. A cut-out's pixel is subject where
its alpha is 128 or more after scaling, and is laid on the background at full opacity, so the mask is exact; its colors stay
as they were, partial fur edges included (AS-2 counts only pixels farther than 4 pixels from the mask's edge).
"""
import os
import sys
import urllib.request

import numpy as np
from PIL import Image

W, H = 960, 720
BASE = "https://upload.wikimedia.org/wikipedia/commons/"
CUTS = {
    "jack-russell": BASE + "2/2d/Transparent-jack-russell-puppy-dog.png",
    "abyssinian": BASE + "7/72/Fawn_Abyssinian_cat_no_background.png",
    "turkish-van": BASE + "9/9b/Turkish_Van_cat_transparent_%28cropped%29.png",
    "shepherd": BASE + "2/2d/German_Shepherd_Transparent.png",
    "exotic": BASE + "6/67/Exotic_cat_transparent.png",
    "kitten": BASE + "a/ab/Orange_and_white_tabby_kitten_%28no_background%29.png",
    "chihuahua": BASE + "d/d2/Chihuahua_dog_for_a_collage.png",
    "toyger": BASE + "c/c5/Toyger_cat_transparent.png",
}


def plain(rgb):
    return np.broadcast_to(np.array(rgb, np.float64), (H, W, 3)).copy()


def vertical(top, bottom):
    t = np.linspace(0, 1, H)[:, None, None]
    return np.array(top, np.float64) * (1 - t) + np.array(bottom, np.float64) * t * np.ones((1, W, 1))


def radial(inner, outer):
    y, x = np.mgrid[0:H, 0:W]
    t = np.clip(np.hypot(x - W / 2, y - H / 2) / np.hypot(W / 2, H / 2), 0, 1)[..., None]
    return np.array(inner, np.float64) * (1 - t) + np.array(outer, np.float64) * t


# Each picture: its background and its subjects as (cut-out, the box it fits in: x, y, w, h[, anchor]). A cut-out whose photo
# ended across the subject is anchored there to the picture's edge, as in a photo; the others are centered in their box.
PICTURES = [
    (vertical((222, 210, 190), (150, 150, 160)), [("jack-russell", (200, 60, 560, 660), "bottom")]),
    (plain((170, 205, 160)), [("abyssinian", (220, 50, 520, 620))]),
    (vertical((120, 140, 190), (200, 190, 220)), [("turkish-van", (260, 20, 440, 680))]),
    (radial((235, 235, 225), (180, 190, 200)), [("shepherd", (0, 120, 780, 600), "bottom-left")]),
    (radial((90, 70, 110), (40, 40, 60)), [("exotic", (40, 140, 480, 440)), ("kitten", (560, 260, 360, 300))]),
    (plain((245, 245, 240)), [("chihuahua", (30, 160, 520, 420)), ("toyger", (580, 120, 360, 460))]),
]


def cut(cache, name):
    path = os.path.join(cache, name + ".png")
    if not os.path.exists(path):
        req = urllib.request.Request(CUTS[name], headers={"User-Agent": "Easel test-set builder"})
        with urllib.request.urlopen(req) as r, open(path, "wb") as f:
            f.write(r.read())
    im = Image.open(path).convert("RGBA")
    return im.crop(im.getchannel("A").point(lambda a: 255 if a >= 128 else 0).getbbox())


def main(cache, out):
    os.makedirs(cache, exist_ok=True)
    os.makedirs(out, exist_ok=True)
    for n, (bg, subjects) in enumerate(PICTURES, 1):
        rgb = bg.round().astype(np.uint8)
        mask = np.zeros((H, W), bool)
        for name, (bx, by, bw, bh), *anchor in subjects:
            im = cut(cache, name)
            s = min(bw / im.width, bh / im.height)
            im = im.resize((max(1, round(im.width * s)), max(1, round(im.height * s))), Image.LANCZOS)
            a = np.asarray(im)
            x, y = bx + (bw - im.width) // 2, by + (bh - im.height) // 2
            if anchor:
                y = H - im.height
                x = 0 if anchor[0] == "bottom-left" else x
            on = a[..., 3] >= 128
            region = rgb[y:y + im.height, x:x + im.width]
            region[on] = a[..., :3][on]
            mask[y:y + im.height, x:x + im.width] |= on
        Image.fromarray(rgb, "RGB").save(os.path.join(out, "%02d.png" % n), optimize=True)
        Image.fromarray(mask).convert("1").save(os.path.join(out, "%02d-mask.png" % n), optimize=True)
        print("%02d.png" % n, "subject %.1f %%" % (100 * mask.mean()))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
