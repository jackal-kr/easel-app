# Permission request: IS-Net general-use weights in Easel

For JOD to send (ADR-0006, open point A). To: the DIS authors (Xuebin Qin et al.), through the contact given in
https://github.com/xuebinqin/DIS (the README's contact address, or an issue on the repository if no address is given).

---

**Subject:** Permission to ship the IS-Net general-use weights in Easel, a free GPL paint program

Dear Dr. Qin and the DIS authors,

I am the developer of Easel, a small paint program for the GNOME desktop on Linux, free software under the GNU General Public
License, version 3 or later. It is distributed without charge as source code, as a Flatpak and as a Debian package.

We would like to add a "Remove background" command that finds the main subject of a picture and makes the rest transparent. In our
comparison on several photos, IS-Net general-use (`isnet-general-use`, from https://github.com/xuebinqin/DIS) gave by far the best
results among the models we can run on an ordinary computer, so we would like to ship its weights with the program, converted to the
ONNX format and run on the user's own machine, with no network access.

The DIS code is under the Apache License 2.0, but we could not find a license for the general-use weights themselves, and the
DIS5K dataset has its own terms of use. Before we ship anything, we would like to ask:

1. May we redistribute the IS-Net general-use weights (as an ONNX file) inside Easel, under the terms you prefer, for any user,
   including users who may use Easel commercially?
2. If yes, is there a license name or a notice you would like us to include with the weights (for example in our about dialog and
   in the package's copyright file)?
3. If you prefer, we can download the weights from a location you name instead of copying them, or ship them as a separate,
   optional package.

Easel will credit DIS and IS-Net and link to your repository and paper (Qin et al., "Highly Accurate Dichotomous Image
Segmentation", ECCV 2022). We will not ship the weights until we hear from you.

Thank you for your work and for considering this.

Kind regards,
JOD
Easel (GNOME paint program)

---

Notes for JOD: send from the address you use for Easel; keep the reply (or its absence) with this file. If permission is refused,
ADR-0006 falls back to silueta through the same code path; if there is no answer before the release that would ship it, the
model is not shipped (the menu item is then unavailable, as the build option of 0035 allows).
