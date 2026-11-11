# 0006 The model that finds the subject (Remove background)

**Status:** Superseded by ADR-0007 (2026-11-09: no model, a classical method; kept for the record). Was: Accepted (2026-11-09, JOD: IS-Net built now, its weights' permission asked before any release that ships it; the budgets of point 5 and the 179 MB)

**Context:**
Spec 0035 needs a model that finds the main subjects of a picture, runs on the machine with no network (REQ-0024-10), ships with the
app, and gives the same pixels on one machine (REQ-0035-07). The constitution keeps `src/core` libc only and asks for a measurement
before threads (P5), and P7.4 asks for the same pixels everywhere. Measured on this machine (AMD Ryzen AI 7 PRO 350, 16 threads,
57 GB), ONNX Runtime 1.31 on the CPU, a photo scaled to the size, the mask thresholded at a half and scaled back; peak memory is the
whole Python process's, so the app's own will be lower. Five system wallpapers compared in `docs/reference/0035/models-sheet.png` (kept locally, not in the repository: it shows third-party wallpapers).

| Model (license of code / weights) | File | 1920 x 1080: 1 thread / all | 8000 x 8000: 1 thread / all | Peak | Sheet |
|---|---|---|---|---|---|
| u2netp (Apache-2.0 / not stated) | 4.6 MB | 0.34 / 0.19 s | not run | 560 MB | loses half the leaf; fails the line art |
| silueta, a pruned U-2-Net (Apache-2.0 / not stated) | 44 MB | 0.82 / 0.52 s | 1.05 / 0.80 s | 1.1 GB | good; loses the stingray's tail |
| IS-Net general-use (Apache-2.0 / not stated; trained on DIS5K, own terms) | 179 MB | 2.45 / 0.99 s | 2.73 / 1.28 s | 2.1 GB | the best: the tail, the whole leaf |
| BiRefNet-lite (MIT / MIT) | 224 MB | 9.3 / 5.9 s | not run | 12 GB | ruled out by its memory |
| BRIA RMBG-2.0 | 1 GB | not run | not run | | ruled out: commercial use needs a paid agreement |

Every model gave the same mask twice in one process, and IS-Net and silueta gave the same mask with one thread and with all (on the
leaf at 8000 x 8000, sampled every 20 pixels). Loading takes 0.2 s for IS-Net. Ubuntu 26.04 packages ONNX Runtime 1.23.2 (universe);
the Flatpak would build it.

**Decision (proposed):**
1. IS-Net general-use, on the condition that its weights may be shipped in a GPL app (open point A); silueta as the fallback if not.
2. ONNX Runtime (MIT) on the CPU, in a new layer `src/ml` outside `src/core`, linked only by the ui; the pixel rules (the threshold at a
   half, the scaling of the mask, removing the background's pixels) stay in `src/core` and are tested headless.
3. The work runs in one worker thread so that the progress dialog and Cancel answer (REQ-0035-04), and ONNX Runtime may use its own
   threads, a fixed number, so the result does not depend on the machine's load (P5: measured, 2.7 s against 1.3 s at 8000 x 8000).
4. The constitution's P7.4 gains: "except Remove background, whose model's arithmetic may differ between processors: the same result
   on one machine and build (ADR-0006)."
5. Budgets proposed for `docs/engineering.md` (open point B): from the choice of the command to the result painted, the first run
   included, at most 2 s for 1920 x 1080 and 3 s for 8000 x 8000 on this machine; at most 2 GB of memory beyond the picture and its step.

**Open points, as settled by JOD (2026-11-09):**
- A. The weights' license. The code of IS-Net and U-2-Net is Apache-2.0, but no license is stated for the general-use weights, and
  DIS5K has its own terms of use (a fork's copy says commercial users sign a free agreement). Before shipping: ask the authors, or
  accept silueta's equally unstated terms, or BiRefNet's MIT at its memory cost. Settled: build with IS-Net; ask the DIS authors for
  written permission before any release that ships it; if refused, silueta through the same code path.
- B. The budgets above, and the download: IS-Net adds 179 MB to the app (an 8-bit version, about 45 MB, would need its quality
  checked against the sheet). Settled: the budgets as proposed and the 179 MB; no 8-bit model for now.

**Consequences:**
`docs/engineering.md` gains the budgets and the `src/ml` layer; the Flatpak and the deb gain ONNX Runtime and the model; 0035's Plan
follows this ADR. Sources: rembg's model list (github.com/danielgatis/rembg), DIS (github.com/xuebinqin/DIS), BiRefNet
(huggingface.co/ZhengPeng7/BiRefNet), Ubuntu's onnxruntime package (packages.ubuntu.com/source/resolute/onnxruntime).
