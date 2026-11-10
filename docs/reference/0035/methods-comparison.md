# Finding the subject: the classical way against three models

For JOD's choice (2026-11-09): **A** classical only, **B** ONNX Runtime and a permissively licensed model, **C** classical by default with
a model as an optional add-on. The masks: `methods-sheet.png` (the five wallpapers of `models-sheet.png`, the hard mask over a
checkerboard). Licences quoted in `model-licences.md`. The classical prototype: `classical/classical-mask.c` (not in the app).

Measured 2026-11-09 on this machine (AMD Ryzen AI 7 PRO 350, 16 threads), the machine busy (load average 9 to 13 during the runs, so
the times are on the slow side). Models through ONNX Runtime 1.31 in Python, the app's own steps (the picture scaled to 1024, the
answer stretched by its range, scaled back, below one half removed); each measurement in a process of its own; "memory" is that
process's peak, the picture loaded before (96 MB at 1920 x 1080, 375 MB at 8000 x 8000). IS-Net inside the app itself (0035 T006,
ONNX Runtime 1.23.2 C, 4 threads): 1.2 to 1.4 s and 1.2 to 2.0 s, at most 1.17 GB beyond the picture.

| | Classical (A) | BEN2 Base | ormbg | IS-Net general-use (built now) |
|---|---|---|---|---|
| Adds to the app | nothing (about 300 lines of C, integers only) | 223 MB model + ONNX Runtime (22 MB) | 176 MB model + ONNX Runtime | 179 MB model + ONNX Runtime |
| 1920 x 1080, first run (load + run), 1 thread / 16 | 0.03 s (single-threaded) | 21.4 / 9.1 s | 2.6 / 1.1 s | 2.5 / 1.0 s |
| 8000 x 8000, first run, 1 thread / 16 | 0.74 s (single-threaded) | 25.0 / 11.3 s | 3.1 / 1.7 s | 2.9 / 1.5 s |
| Memory, whole process at 1920 x 1080 / 8000 x 8000 | 95 MB / 516 MB | 7.7 / 8.5 GB | 1.6 / 2.6 GB | 1.6 / 2.6 GB |
| Within 2 s / 3 s / 2 GB beyond the picture | yes | no (time and memory) | yes (as IS-Net: the same network shape) | yes (T006) |
| 0035's AS-2 on its six test pictures (95 % rule) | 5 of 6 (01: 28 % of the white dog kept) | 6 of 6 | 6 of 6 | 6 of 6 |
| The five wallpapers (the sheet) | mug and leaf good with stray pieces; raccoon whole; stingray not found; the twins' drawing filled in | good, stray marks on the raccoon's wallpaper | stingray in pieces, the twins' lines mostly lost (it is made for people) | the best: stingray with its tail, the leaf alone, the twins' lines |
| Same pixels on every machine (P7.4) | yes: integers only, no exception needed | no: the exception of ADR-0006 | no | no |
| Same pixels twice on one machine | yes | yes | yes | yes |
| Weights licence, as stated | none needed | "license: mit" | "license: apache-2.0" | not stated |
| Training data, as stated | none | "the DIS5k and our 22K proprietary segmentation dataset" | synthetic humans; "based on" the DIS research | DIS V1.0 (DIS5K: "non-commercial use in research or educational purpose") |
| Builds without sudo | yes (no dependency) | yes, with ONNX Runtime's official archive in the home folder (`~/.local/opt/easel-dev-env.sh`); a distribution's package needs sudo | same | same |
| Flatpak | nothing to add | ONNX Runtime as a module (T001: the 8.3 MB archive runs in the GNOME 51 sandbox) | same | same |

## What the numbers say
- The classical prototype is fast, small and exact everywhere, and passes five of the six test pictures, but it fails where the
  subject is close to the background's colors or soft-edged (the white dog on a pale background, the stingray in the water) and
  takes along pieces that touch neither the border nor a wall. It is good on flat backgrounds: screenshots, logos, cut-outs.
- BEN2 Base, the permissive model, is out on cost: about nine seconds and almost 8 GB.
- ormbg is IS-Net's size and speed with a clear Apache-2.0 tag, but it is trained for people and drops the stingray and line art.
- IS-Net is the best and within budget, but its weights' licence is not stated (permission asked, letter drafted) and DIS5K's terms
  speak of non-commercial use.
- Every model names DIS5K or builds on it (BEN2 too, besides its own data), so a permissive tag on the weights does not end the
  training-data question for B.

## The classical method improved (v2, 2026-11-09, JOD's request)

`classical/classical-v2.c`, libc and integers only, four stages each adding to the one before: (1) colors compared in an integer Lab
approximation (delta-E) instead of RGB; (2) the background as a cubic surface per Lab channel fitted to the border ring by fixed-point
least squares (refitted once without the samples far from the first fit) instead of three flat colors; (3) a marker-based watershed on
the Lab edge strength (a bucket queue, first in first out, so deterministic): background seeds in the border ring near the surface,
subject seeds anywhere at least 4 tolerances from it; (4) the largest subject region kept with any other at least a tenth of its size
(pictures 05 and 06 have two subjects, so "the largest only" would drop one), enclosed background under 1 % of the picture filled.
Each stage at its best single tolerance on the six test pictures (delta-E: stages 1 and 2 at 8, stages 3 and 4 at 5). Sheets:
`classical-v2-sheet.png` (the first prototype and each stage on the six test pictures and the five wallpapers),
`classical-v2-tolerance.png` (stage 4 on the wallpapers at tolerances 5, 10, 15, 20 and 30).

Test set, the share of the subject kept / of the background removed, in %, counted 4 pixels from the true mask's edge (AS-2's 95 % rule):

| Picture | First prototype | Stage 1 | Stage 2 | Stage 3 | Stage 4 |
|---|---|---|---|---|---|
| 01 white dog, pale gradient | 28.1 / 100.0 | 59.1 / 74.3 | 99.1 / 100.0 | 98.9 / 100.0 | 98.9 / 100.0 |
| 02 cat on green | 100.0 / 99.9 | 100.0 / 99.9 | 100.0 / 99.9 | 100.0 / 99.9 | 100.0 / 99.9 |
| 03 white cat, blue gradient | 100.0 / 100.0 | 100.0 / 74.5 | 100.0 / 100.0 | 100.0 / 100.0 | 100.0 / 100.0 |
| 04 dog's head, pale radial | 100.0 / 100.0 | 100.0 / 100.0 | 99.9 / 58.7 | 99.8 / 92.3 | 99.8 / 92.3 |
| 05 two cats, dark radial | 100.0 / 100.0 | 100.0 / 71.3 | 100.0 / 77.0 | 100.0 / 93.9 | 100.0 / 93.9 |
| 06 dog and cat, plain | 100.0 / 100.0 | 100.0 / 100.0 | 100.0 / 100.0 | 100.0 / 100.0 | 100.0 / 100.0 |

Wallpapers (1920 x 1080), the share of the picture kept at those tolerances; at a tolerance chosen per picture (the dialog's value)
stage 4 keeps the raccoon alone at 15 to 20, the mug and its steam alone at 15 to 20 (the cleanest of all versions), the leaf alone at
15 to 20; the twins' line art goes above 10; the stingray is never found cleanly (at 5 it comes with a patch of water, above 10 not at all).

| Wallpaper | First prototype | Stage 1 | Stage 2 | Stage 3 | Stage 4 |
|---|---|---|---|---|---|
| raccoon | 3.8 | 76.5 | 78.3 | 70.6 | 66.3 |
| coffee mug | 14.5 | 19.1 | 67.8 | 55.0 | 55.0 |
| stingray | 0.0 | 23.0 | 16.8 | 25.2 | 24.9 |
| twin brothers | 38.5 | 86.5 | 80.5 | 67.6 | 67.6 |
| red acer leaf | 13.1 | 77.2 | 86.7 | 87.2 | 86.8 |

Time and peak memory (the whole process, the picture loaded; load average 1.6 to 1.9), single-threaded:

| | First prototype | Stage 1 | Stage 2 | Stage 3 | Stage 4 |
|---|---|---|---|---|---|
| 1920 x 1080 | 21 ms, 18 MB | 32 ms, 23 MB | 38 ms, 24 MB | 88 ms, 23 MB | 129 ms, 30 MB |
| 8000 x 8000 | 0.71 s, 516 MB | 0.98 s, 656 MB | 1.10 s, 698 MB | 4.09 s, 672 MB | 4.53 s, 885 MB |

Reading: on cut-out subjects on plain or smooth backgrounds the improved method removes the white dog's limit (98.9 % kept against
28.1 %) and holds 92 to 94 % on the radial backgrounds (just under AS-2's 95 %); on real wallpapers no single tolerance suits all, but a
tolerance set per picture gives clean results on three of five. The watershed stages are over the 1 s budget at 8000 x 8000 (4.1 to 4.5
s) and every version holds well over the picture in memory (Lab copy, labels, a queue); as a prototype it does not try to be cheap
(the surface is evaluated per pixel, the edge strength twice).

Not used: GrabCut, or any iterated graph cut with color models (JOD, 2026-11-09: believed to be covered by a patent, US 7,660,463;
patent status unchecked, not researched further).

## As it is to be built (v3, 2026-11-09, JOD: stage 4, automatic tolerance, made cheap)

`classical/classical-v3.c`: stage 4 of v2 run on a copy of at most about 2 million pixels (blocks of s x s averaged, integer means;
the picture itself when smaller), the surface, the edges and each pixel's distance to the surface computed once; the result at full
size copies the copy's label where a cell and its eight neighbours agree, and on the boundary takes the label of the neighbouring
cell nearest in Lab (ties: subject); the preview is the copy's mask repeated. The automatic tolerance comes from the median distance
of the ring's samples to the fitted surface (the median, so that a subject crossing up to half the ring does not raise it): 5 while
that median is at most 2 delta-E, then 2 more per delta-E, at most 20. `classical-v2-sheet.png` shows it in its last column with the
tolerance it chose.

Test set, automatic (subject kept / background removed): 01 at 5: 98.9 / 100.0; 02 at 5: 100.0 / 99.9; 03 at 5: 100.0 / 100.0; 04 at 7:
98.4 / 93.3; 05 at 5: 100.0 / 93.9; 06 at 5: 100.0 / 100.0. With the tolerance set: 04 at 8: 96.4 / 98.6 and 05 at 7: 100.0 / 100.0, so
every picture clears 95 / 95 at its own tolerance. No single automatic rule passes all six: the white dog needs 5 or less and the two
cats 7 or more, and their borders' medians (0.61 and 0.86 delta-E) cannot tell them apart; 5 was chosen, the cats' miss being the
smaller (93.9 % against 81 % of the dog at 8).

Wallpapers, automatic: raccoon 13, mug 13, stingray 7, twins 5, leaf 20: the raccoon, the mug with its steam and the leaf come out
alone; the stingray's body is found without its tail; the twins' drawing is filled in (a lower tolerance in the dialog).

Time (single-threaded) and memory (the whole process, the picture loaded): 1920 x 1080: the preview 28 ms, the result 47 ms, peak
17 MB; 8000 x 8000: the preview 124 ms, the result 0.71 s, peak 273 MB of which 192 MB are the picture and 64 MB the full-size mask:
within JOD's targets (the preview 0.3 s at any size; the result 1 s at 1920 x 1080, 2 s and 1 GB beyond the picture at 8000 x 8000).

Added in the app (2026-11-09, found by the core test of a red disk on white): the seam. Both sides of a color step have the same edge
strength, so the subject's flood took the first background pixel beside it, a one-pixel fringe round every subject. A subject cell beside
background whose color is within the tolerance of the surface now becomes background. The test set's numbers above are unchanged (the
fringe lies inside the 4-pixel margin of the count).

The fringe (2026-11-10, JOD's eagle over a blue sky, a photo whose licence is not ours, so it is measured here and shown on no sheet): a dark-blue band of mixed pixels stayed along the motion-blurred wings, up to 14
pixels wide. Tried in turn: removing boundary pixels near the surface (one pixel a pass: the band stays); the same repeated (an
erosion: JOD declined it); a blend test in Lab against the pixel 3 inside (the inside of a soft edge is itself a blend, and in Lab the
mix bends, so a straight blend is not recognised); the subject as the farthest pixel inward (it reached darker detail and ate whole
patches of the raccoon and the leaf); the background as the removed neighbour (itself a blend, so each round moved inward). As built:
per axis direction, both colors where the color has settled, the test in RGB, the fringe kept to an eighth of the colors' distance
from their line, one round. JOD asked for rounds while one removes, at most 3 or 4: a second round walks outward over the blends the
first removed, so its background is itself a blend and it bites into the subject (a noisy soft edge: 30 to 194 subject pixels
removed with 3 rounds, none with one; the tabby's legs 877 against 506), while it adds little to the eagle (15 203 against 12 727). On the six test pictures it removes 0.0 to 0.9 % of what was kept, a pixel or two along the outline; on
the wallpapers 0.2 to 0.8 %; the twins' line art, the leaf's veins and the cats' whiskers stay; the eagle loses 4.4 %, its blue band.
Small bites remain where pale fur meets the background beside a dark stripe (the tabby, 06) and the blurred wing tips keep a few
hair-thin streaks. AS-2 at the named tolerances: 04 at 8: 96.3 / 98.6, 05 at 7: 100.0 / 100.0 (unchanged within 0.1). Time: see the spec's Plan. Sheet: `fringe-sheet.png`.

