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

## 0037 B: the background in segments (prototype, 2026-11-10, JOD's request; not in the app)

`classical/classical-v4-segments.c`: the app's method with the background surface fitted in up to three segments of the border ring,
split by color. One surface first, as now; only when it leaves 10 % of the ring more than 2 automatic tolerances from it are the ring's
colors split (integer k-means from the farthest points, 3 then 2), a split kept when every segment holds 10 % of the ring, owns one of
its corners and lies 15 delta-E from the others; each segment its own cubic surface, a cell's distance to the background to the
nearest one. Sheet: `segments-sheet.png` (the picture, the app, segments; automatic tolerance). The horizons h1 and h2 are synthetic
(a sky gradient over noisy green ground, a figure across the horizon, a small bird; truth by construction), since the test set has
none.

| Picture (automatic tolerance) | One surface (the app): kept / removed | Segments: kept / removed |
|---|---|---|
| h1 horizon, 1920 x 1080 | 92.9 / 70.3 | 92.9 / 76.1 (2 segments) |
| h2 horizon, tilted | 92.9 / 71.8 | 92.9 / 74.8 (2) |
| h3 horizon, 8000 x 8000 | 92.8 / 68.7 | 92.8 / 94.0 (2) |
| 04 dog's head, pale radial | 98.4 / 93.3 (at 7) | 98.8 / 100.0 (2, at 5) |
| 01, 02, 03, 05, 06 | as before | the same pixels (1 segment) |
| the five wallpapers, the eagle | as before | the same pixels (1 segment) |

(The kept share 92.9 on the horizons is the small bird, dropped by REQ-0035-03 c as under a tenth of the figure.)

Time (single-threaded) and the process's peak memory, the picture loaded: 1920 x 1080: analysis and labels 38 ms -> 59 ms, the result
33 ms -> 32 ms, 32.7 MB -> 33.3 MB; 8000 x 8000: analysis and labels 157 ms -> 220 ms, the result 908 ms -> 904 ms, 724 MB -> 728 MB.
The opening's preview at 8000 x 8000 (186 to 276 ms with one surface) would come to about 250 to 340 ms: at or over the 0.3 s budget.

What did not work, on the way: splitting every ring whose colors form clusters (the white dog's gradient split in two; extrapolated, a
segment's surface passed near the dog's white: 37.9 % of it kept); planes instead of cubics for the segments (78.8 %); requiring each
segment to own a corner (the gradient's two halves both do); each segment its own automatic tolerance (04 fell to 82 to 87 % kept,
the horizon's ground got 5 as well); background seeds anywhere for segments (79.8 % on h1, 04 at 82 %).

Reading: segments help where a surface cannot follow a step along the border (the horizon at 8000 x 8000, 04's radial), and change
nothing where one surface suffices. At 1920 x 1080 the horizon's ground stays mostly because of its texture (per-pixel noise above
tolerance 5): at tolerance 10 one surface removes 100 % of h1 too. So most of a horizon photo's gain comes from the tolerance, not the
surface; the segments' own gain is the step itself.

## The cube root and the re-tune (2026-11-10)

The integer cube root behind the Lab of every version since v2 was wrong (the step's test computed before the shift, 3 r (2 r + 1) + 1
for 3 y (y + 1) + 1): 64 944 of the 64 956 values of f(t) were off, the green (37, 112, 56) came out with a* = 0 instead of about -40.
All the delta-E numbers in the sections above were measured on that Lab. Fixed in the app and the prototypes, with an oracle test
against libm. On a correct Lab the old automatic rule (5, then 2 per whole delta-E over 2) left picture 04 at 87.1 % kept; every
picture was then run at every Tolerance from 1 to 40, and the rule chosen for the worst subject kept, the test pictures' background
removed no lower than before, the photos without truth no fuller than before by more than 2 points:

| Picture | Before the fix (tolerance: kept / removed) | Fix only | Re-tuned (5, then 3 per delta-E over 3.05) |
|---|---|---|---|
| 01 white dog | 5: 98.9 / 100.0 | 5: 97.2 / 100.0 | 5: 97.2 / 100.0 |
| 02 | 5: 100.0 / 99.9 | 5: 100.0 / 99.9 | 5: 100.0 / 99.9 |
| 03 white cat | 5: 100.0 / 100.0 | 5: 100.0 / 100.0 | 5: 100.0 / 100.0 |
| 04 dog's head | 7: 98.4 / 93.3 | 7: 87.1 / 96.8 | 6: 97.5 / 95.8 |
| 05 two cats | 5: 100.0 / 93.9 | 5: 100.0 / 100.0 | 5: 100.0 / 100.0 |
| 06 | 5: 99.9 / 100.0 | 5: 100.0 / 100.0 | 5: 100.0 / 100.0 |
| green synthetic, more light | 11: 34.9 / 74.6 | 5: 94.4 / 64.6 | 5: 94.4 / 64.6 |
| JOD's green-backdrop portrait (shirt and arms / backdrop) | 19: 47.2 / 98.6 | 7: 100.0 / 98.6 | 5: 100.0 / 98.5 |
| the eagle (share kept) | 5: 12.1 % | 5: 10.9 % | 5: 10.9 % |
| leaf wallpaper (share kept) | 20: 5.7 % | 11: 7.7 % | 11: 7.7 % |
| mug wallpaper (share kept) | 13: 15.3 % | 7: 26.8 % | 6: 39.2 % |

Three rules were offered to JOD (floor 3 and 3 per delta-E over 2.5, the best on every picture with a truth but the photos' noise
left: the eagle 35.7 % kept; floor 5 and 1 per delta-E over 2.5; and this one); he chose this one. Its knee sits in the third of a
delta-E that picture 04 needs (5 removes 51 %, 7 keeps 87 %), centred, so 04's median (3.55) has 0.17 delta-E either way; the first
estimate keeps its own fixed rule so that the shown rule does not move the median it reads. No rule fixes the mug: its median (3.62) is
04's and it needs about twice the tolerance. Sheet: not committed (it shows JOD's photos); the numbers are above.

## v5 folded in (2026-11-10): the robust fit and the colour line

Prototype `classical/` (scratch, not kept; the app's `src/core/remove-bg.c` is the method). On the correct Lab, stage by stage, JOD's
order; the synthetic green backdrops and the portrait are measured by region (shirt, arms, backdrop):

| Stage | Shirt kept (green synthetic / portrait) | Glow removed (green synthetic) | Test set: worst kept / worst removed | Verdict |
|---|---|---|---|---|
| the app, Lab fixed | 91.3 / 100 | 0 | 97.2 / 95.8 | |
| (a) robust fit | 100 / 100 | 0 | 97.6 / 97.4 | kept |
| (a) + rejected ring cells as subject seeds | 100 / 100 | 0 | 97.7 / 96.7 (at 2 tolerances: 50.7) | dropped |
| (a) + (b) colour line | 100 / 100 | 99.7 | 97.6 / 99.4 | kept |
| (a) + (b) + (c) enclosed regions by color | 100 / 100 | 99.7 | 97.6 / 99.4 | dropped: the portrait's gap 98.6 to 99.0 % at Tolerance 10 only |

The line's guards, each tried by removing it alone on 22 pictures: "only where the line explains the ring at least as well as the
surface" changes 7 (without it the stingray and the twins' line art go whole, being shades of their backdrops); "only colored cells"
keeps a white subject on a light colored backdrop (the core test `line_guards`); a cap where a channel saturates changed no picture,
and "no line on a grey backdrop" made picture 04 worse (100 / 99.4 without it, 98.5 / 97.4 with): both dropped. Also dropped: Otsu's
tolerance (the white dog at 0 to 26 % at every mapping), the majority filter (nothing gained), lightness weighted below chroma (nothing
gained), the power method's refit of the line (the same pixels as the ray, and it rotated toward the subject on the portrait).

The knee of the automatic tolerance, swept from 2.6 to 3.6 delta-E: AS-2 holds over the whole range (a plateau of 1 delta-E at least);
with the robust fit no test picture's median is near the knee any more (the largest, 04, is 0.63); only the leaf (5.1) and a synthetic
horizon move with it.

Held out (drawn here, truth by construction, not used for any choice): a plain wall 100 / 100; a sky with soft clouds 100 / 94.7; a
studio grey with a falloff 100 / 100; a two-color gradient 100 / 100; a white backdrop with a soft shadow 100 / 94.8; a textured
backdrop 0 / 100 at the automatic tolerance 20, 100 / 99.5 at 10 to 12 (a known limit in the spec).

Time (perf-0035, the machine at load 11): 8000 x 8000: the opening with its preview 240 to 280 ms, a Tolerance change 141 to 143 ms,
the result 0.93 to 0.95 s, the analysis 50 MB; 1920 x 1080: 41 to 46 ms, 19 ms, 45 to 47 ms.

## v6: complex backgrounds (2026-11-10, JOD's request; one round)

`classical/classical-v6.c`, `v6-sheet.png` (synthetic pictures only). New synthetic backdrops, truth by construction: a two-color split,
stripes, a strong noise texture, a gradient with a horizon, a backdrop the subject partly matches; with the held-out six and the test
set. Subject kept / background removed, %, at the automatic tolerance:

| Picture | The app before v6 | (1) clusters, kept where they fit the ring better | (1) without that guard | (1) + (2) histograms | (1) + (2) + (3) refits |
|---|---|---|---|---|---|
| two-color split | 100 / 57.0 | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| stripes | 100 / 51.9 | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| noise texture | 0 / 59.4 (tolerance 20) | 0 / 100 | 0 / 100 | 0 / 94.0 | 0 / 94.0 |
| horizon | 100 / 74.0 | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| partly matching | 89.9 / 100 | 79.2 / 100 | 79.2 / 100 | 79.2 / 100 | 79.2 / 100 |
| held-out sky | 100 / 94.7 | 100 / 94.7 | 100 / 100 | 100 / 89.2 | |
| held-out white backdrop | 100 / 94.8 | 100 / 94.8 | 100 / 94.8 | 100 / 95.6 | |
| 01 white dog | 97.6 / 100 | 97.6 / 100 | 28.1 / 100 | 93.8 / 93.9 | 29.5 / 100 |
| 03 white cat | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| the twins' line art (share kept) | 28.3 | 28.3 | 0.0 | 50.5 | 0.0 |
| thin lines (core test) | intact | intact | intact | intact | intact |

The rest of the test set, the wall, the studio, the gradient and the photos (wallpapers, the eagle, the portrait) are the same pixels
with the kept clusters (the mug 14.3 -> 13.9 % kept). Kept: (1) with the guard. Dropped: the histograms (2) and their refits (3), which
bring nothing the clusters do not and cost the white dog (93.8 %, under JOD's 97 %); a corner rule for the clusters (it lost the
stripes). The partly matching backdrop loses the shirt's part in the backdrop's own green (a press brings it back, Remove Background:
pieces); the noise texture keeps its known limit (the automatic tolerance 20 loses the subject; 10 to 12 is right). Texture and
superpixels were not tried: after the clusters, no backdrop here fails for want of them but the noise texture, already a documented
limit with a working Tolerance.

Time (single-threaded) and memory: 1920 x 1080, the analysis 50 -> 52 ms, the result 28 ms, 34.7 -> 35.1 MB; 8000 x 8000, the analysis
192 -> 210 ms, the result 0.84 -> 0.83 s, 731 -> 733 MB (the process, the picture loaded).

## The centre prior, the ring sides and the confidence hint (2026-11-10)

A selfie (a hat over a garden; the dark shirt fills the whole bottom border; JOD's photo, kept local) loses its lower face, beard
and shirt at the automatic 12. Cause: the ring's bottom edge is the shirt, so it is seeded as background and the robust surface takes
it in (residual 5.41 dE, the colour line off, no clusters). The v6 clusters play no part. A synthetic equivalent (a dark block
filling the bottom border with a lighter shape above it) keeps 0.0 %; it is AS-11's picture.

**Centre prior, dropped** (a disc 5 % of the shorter side round the centre, seeded as subject, never as background). The floors break:

| Picture | Now | Prior | Prior, only where farther than T |
|---|---|---|---|
| t5 two cats (AS-2) | 100.0 / 100.0 | 100.0 / 91.7 | 100.0 / 91.7 |
| t6 (AS-2) | 100.0 / 100.0 | 100.0 / 89.3 | 100.0 / 100.0 |
| t2 (AS-2) | 100.0 / 99.9 | 100.0 / 94.5 | 100.0 / 99.9 |
| held-out studio | 100.0 / 100.0 | 100.0 / 83.7 | 100.0 / 83.7 |
| held-out white | 100.0 / 94.8 | 100.0 / 83.8 | 100.0 / 94.8 |
| held-out sky | 100.0 / 94.7 | 100.0 / 91.6 | 100.0 / 94.7 |
| noise (synthetic) | 0.0 / 100.0 | 69.2 / 89.2 | 69.2 / 89.2 |
| textured (held-out) | 0.0 / 100.0 | 100.0 / 99.6 | 100.0 / 99.6 |
| synthetic selfie | 0.0 / 99.8 | 23.0 / 100.0 | 23.0 / 100.0 |
| selfie (kept share) | 18.6 % | 34.7 % (upper face) | 34.7 % |

Unchanged: t1 white dog 97.6, t3 white cat 100.0, t4, split, stripes, horizon, match, wall, gradient, thin lines intact. The centre is
background in t5 (the gap between the cats) and in studio (the bright middle of the gradient); the prior keeps it. On the selfie it
brings back the upper face only; the beard and shirt stay removed, since they still touch the background seeds on the border.

**Confidence.** The number of border clusters does not separate the cases: the plain wall has 6 and split 5, both 100 / 100. The ring's
residual does: above 3 dE for split (3.88), the selfie (5.41), textured (11.4), noise (25.4), the synthetic selfie (26.1); at or under
2.16 for the other pictures, the eagle (1.36) included. Match (2.16, 79.2 kept) is a miss of it, accepted. Adopted: the card's second line above 3 dE (REQ-0035-04, AS-11).

**0037 on the selfie.** At 12 the removed area (face, beard, shirt, sky, foliage, house) is one connected piece: one press on the face
brings back all of it but a spot by the eye. The press cannot separate the face from the foliage.

**The eagle.** JOD's eagle at automatic 5 leaves cloud islands; 7 is clean. A mild miss of the automatic rule, no change.

**Ring sides, dropped** (one round: a frame side is not background when more than half of its colours lie farther than 10 dE from
every colour of the other three sides combined). On the selfie no side qualifies: the bottom side's share of such colours is 0.0 %,
since the shirt also runs up the lower ends of the left and right sides (top 0.1, left 0.1, right 5.1 %). The synthetic selfie is the
same (0.0 / 0.0 / 0.4 / 0.3 %). It changes nothing on the selfie, so it was not run over the rest. A subject that fills a side also
reaches the two sides beside it; telling it from the backdrop needs the user's marks (Keep this / Remove this).

## A blind composited benchmark (2026-11-10, local only)

The core frozen at sha256 `818cf0ab…a771b` of `src/core/remove-bg.c` (on commit `46f9ab5` plus the uncommitted work), no change made
for the set. 13 subjects (the nine cut-outs of the test set's sources and four synthetic: a head and shoulders across the bottom edge,
colour blocks, line art, a pale vase) on 12 backdrops generated in code (plain, linear, glow, vignette, two-colour split, horizon,
stripes, noise, bokeh, busy pattern, soft clouds, paper) at 800 x 600, 1600 x 1200 and 3200 x 2400: 468 runs at the automatic
tolerance, the masks exact (the cut-outs' alpha at one half). Kept is the subject kept, removed the background removed, both away
from a 4-pixel band round the true edge; IoU over all pixels. Nothing of it is in the repository; the generator and the sheet stay
local.

| Backdrop | Median IoU | Worst IoU | IoU > 0.9 | Hint shown |
|---|---|---|---|---|
| plain | 0.991 | 0.369 | 85 % | 0 % |
| linear | 0.991 | 0.328 | 85 % | 0 % |
| glow | 0.993 | 0.386 | 92 % | 0 % |
| vignette | 0.994 | 0.386 | 92 % | 0 % |
| paper | 0.995 | 0.377 | 92 % | 0 % |
| split | 0.990 | 0.000 | 72 % | 8 % |
| stripes | 0.966 | 0.000 | 62 % | 8 % |
| horizon | 0.362 | 0.111 | 10 % | 5 % |
| clouds | 0.344 | 0.000 | 31 % | 8 % |
| busy | 0.292 | 0.035 | 0 % | 0 % |
| bokeh | 0.175 | 0.000 | 0 % | 46 % |
| noise | 0.000 | 0.000 | 3 % | 100 % |
| all | 0.932 | 0.000 | 52 % | 15 % |

The hint (median above 3 dE) shows on 65 of the 225 runs under 0.9 (29 %), on 3 of the 243 above, on none of the 39 plain ones.
The 225 runs under 0.9, sorted: known limits of 0035: the hole showing the background (the line-drawn dog whose open interior
shows the backdrop, and the gap between legs, 41), the textured backdrop (35), the horizon's ground (29: 0037's band, item I's
texture), soft clouds (21, far worse than the 95 % the edge case states), a subject in the backdrop's own shade (line art on dark
stripes, 3). New: (A) a subject that fills a border side (the head and shoulders, 24; the selfie's case, 0038's Keep stroke);
(B) two colours each near half of the ring (split, stripes, 6): the surface's median falls between them, the automatic tolerance
jumps to 20 and the subject goes, which side of half depending on the size; (C) a busy pattern of many small colours (33): the
ring's median is the base color, so the hint stays off and every piece of another colour stays; (D) blurred multicolour bokeh (33).

What it can claim: on smooth backdrops (plain, gradients, glow, vignette, paper) the method keeps a cut-out subject well (median
IoU 0.99, 85 to 92 % above 0.9), independent of the picture's size; the hint never shows on a plain backdrop. What it cannot claim:
it is not photographs (the subjects are pasted with their own alpha, no shadow, no blur, no light from the scene); the cut-outs are
the test set's own animals; the backdrops' kinds and parameters were chosen here and their share of the set decides the totals
(52 % above 0.9 says nothing of a user's pictures); and the head and shoulders and the line drawing are hard cases by design. The
hint misses most failures that are not texture. Rules proposed from it are for JOD to decide; none is made to pass the set.

JOD's own 24 photos (local only, never shown before, run once with the frozen core): 5 good (flowers on black, a candle, a tiger
on sky, a clown on dark, a portrait on grey), 10 removed whole at an automatic 16 to 20 (busy or textured scenes: gardens, a street,
foliage, a field, a car under trees, a bar), the rest partial (a subject that fills a border side: faces, a close-up panda; bokeh).
The hint shows on 16 of the 19 failures and on none of the 5 good. On photos the main failure is the automatic tolerance driven to
20 by a busy border, which removes everything.

Three rules tried against the frozen core, one round each (JOD 2026-11-10, "show me all"): (1) the automatic tolerance from the
clusters' median when they judge: composited 22 runs better, 2 worse (line art over a horizon), share above 0.9 52 to 53 %; photos 8
changed, none worse (the astronaut 0 to 31 % kept, the close-up panda 0 to 43 %); every test-set floor as before. (2) The hint from the
ring's 90th percentile distance above 6 dE: 80 % of the composited failures (from 29 %), 13 of 243 good runs, no plain one; on the
photos 19 of 19 failures but 2 of 5 good. (3) Clusters from 3 % of the ring: rejected, it breaks the floors (the split, stripes and
horizon synthetics lose their subject, the studio and match pictures too) and empties three more photos.

### Rules 1 and 2 folded, and a stability check prototyped (2026-11-10)

Rule 1 (the automatic tolerance from the clusters' median where they judge) and rule 2 (the hint from the ring's 90th percentile
above 6 dE) are folded into 0035. Every floor holds: AS-2's six at automatic 5 (97.6 / 100 worst), the white dog 97.6, the white cat
100, thin lines intact, the core tests. Rule 3 stays rejected (above). A grey-grain backdrop now gets a low automatic tolerance (the
clusters explain grain of one hue; the core's noisy-border test moved to a colour grain to keep testing the formula's slope).

JOD's photos split for this round (stratified by the frozen core's labels; the split and its seed in the local scratch notes only):
12 for tuning, 12 locked and run once at the end. The stability check (prototype, not folded): when the automatic tolerance is above
7, the kept share of the copy is taken at 4 and at the automatic tolerance; only if it more than halves, at 7, 10 and 14 in turn, and
the tolerance becomes the last one before the first step that more than halves it. The same picks as the full ladder on every run.

| | Frozen core | Rule 1 | Rule 1 + stability |
|---|---|---|---|
| Composited: median IoU / share above 0.9 | 0.932 / 51.9 % | 0.960 / 52.8 % | 0.956 / 52.4 % |
| Composited: runs left empty (IoU < 0.01) | 57 | 35 | 3 |
| Composited: runs worse than the frozen core by 0.05 | - | 2 | 4 |
| Tuning 12: removed whole / good | 6 / 2 | 4 / 2 | 0 / 3 |
| Locked 12 (run once): removed whole / good | 5 / 3 | 4 / 3 | 0 / 3 (the horse nearly) |
| Floors (AS-2, dogs, cats, thin lines, core tests) | hold | hold | hold |

What the stability check costs and does: one more label of the copy when the automatic tolerance is above 7 (about 110 ms at
8000 x 8000, 30 ms at 1920 x 1080), four more when a collapse must be found (about 440 ms at 8000 x 8000, over the 0.3 s preview
budget, 100 ms at 1920 x 1080). It turns "everything removed" into "something kept", sometimes too much: the street keeps its
buildings, the tiger its foliage, the noise and textured backdrops keep most of themselves (removed 15 % and 8 %). Its four worse
composited runs: two noise backdrops at 3200 and the line art over a horizon (rule 1's, a known limit).
