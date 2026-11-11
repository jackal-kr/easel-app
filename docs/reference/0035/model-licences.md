# Models that could find the subject: code and weights licences

For JOD's choice between A (classical only), B (ONNX Runtime and a model with a clear permissive licence) and C (classical by default,
the model as an optional add-on). ONNX Runtime itself is MIT, so only the weights' licence and their cost count here.

Checked 2026-11-09, from the sources linked in each row (the repositories' `LICENSE` files and READMEs as served by
raw.githubusercontent.com, the Hugging Face model cards' front matter and the Hugging Face API). Everything legal is quoted;
"not stated" means the source says nothing on it. Speed and memory are this machine's from ADR-0006 (AMD Ryzen AI 7 PRO 350,
ONNX Runtime 1.31 in Python, peak memory of the whole Python process) where a model was measured, else "not measured" (no model card
here publishes a memory or CPU speed figure); BEN2 Base and ormbg were measured the same day with the same steps, see `methods-comparison.md`. The last column is an engineering reading of the quoted terms, not legal advice.

## Summary

| Model | Code licence | Weights licence (quoted) | ONNX file | 1920 x 1080 (1 thread / all) | Peak memory | Weights in a GPL-3.0 app |
|---|---|---|---|---|---|---|
| IS-Net general-use (built now) | Apache-2.0 | not stated | 179 MB | 2.45 / 0.99 s | 2.1 GB | unclear: permission asked (`ask-dis-authors.md`) |
| u2netp | Apache-2.0 | not stated | 4.6 MB | 0.34 / 0.19 s | 560 MB | unclear: no licence for the weights |
| silueta | Apache-2.0 (U-2-Net's) | not stated | 44 MB | 0.82 / 0.52 s | 1.1 GB | unclear: a quantized U-2-Net, same status |
| BiRefNet-lite | MIT | "license: mit" | 224 MB | 9.3 / 5.9 s | 12 GB | yes as stated (MIT); ruled out by memory |
| BiRefNet (full) | MIT | "license: mit" | 1.0 GB (withoutBG's copy) | not measured | not measured | yes as stated (MIT); size |
| BEN2 Base | not stated (card only) | "license: mit" | 223 MB | 21.4 / 9.1 s (first run) | 7.7 GB | yes as stated (MIT); ruled out by time and memory |
| ormbg | Apache-2.0 (card) | "license: apache-2.0" | 176 MB | 2.6 / 1.1 s (first run) | 1.6 GB | yes as stated, with a question (see below) |
| InSPyReNet | MIT | not stated | none published (`.pth`, 368 MB) | not measured | not measured | unclear: no licence for the weights |
| MODNet (portraits only) | Apache-2.0 | "The code, models, and demos in this repository … are released under the Apache License 2.0" | none published (an export script in `onnx/`) | not measured | not measured | yes as stated; portraits only |
| withoutBG Open Weights | Apache-2.0 (SDK) | "Apache-2.0 AND LicenseRef-DINOv3" | 104 MB + 350 MB backbone (+ 1.0 GB BiRefNet branch) | not measured | not measured | doubtful: DINOv3's terms pass to every recipient |
| BRIA RMBG-1.4 | not stated | "source-available model for non-commercial use" | 176 MB | not measured | not measured | no |
| BRIA RMBG-2.0 | not stated | `license_link: https://creativecommons.org/licenses/by-nc/4.0/deed.en` | 1.0 GB | not measured | not measured | no |

## Training data, as stated

A permissive licence on the weights does not settle the terms of the data they were trained on (JOD, 2026-11-09). Quoted from the same
sources, checked the same day:

| Model | Training data, as stated |
|---|---|
| IS-Net general-use | "the released DIS deep model is the academic version that was trained with DIS V1.0" and "The Terms of use for our DIS5K dataset is provided as [DIS5K-Dataset-Terms-of-Use.pdf]" (https://github.com/xuebinqin/DIS); for the general-use file itself: not stated |
| u2netp, silueta | not stated for these files; the README speaks of "u2net trained with DUTS-TR dataset" (https://github.com/xuebinqin/U-2-Net) |
| BiRefNet, BiRefNet-lite | "This BiRefNet for standard dichotomous image segmentation (DIS) is trained on **DIS-TR** and validated on **DIS-TEs and DIS-VD**." (both cards); DIS-TR is DIS5K's training part |
| BEN2 Base | "BEN2 was trained on the DIS5k and our 22K proprietary segmentation dataset." (https://huggingface.co/PramaLLC/BEN2) |
| ormbg | "I started training the model with synthetic images of the [Human Segmentation Dataset](https://huggingface.co/datasets/schirrmacher/humans) crafted with [LayerDiffuse]" and "It is based on [Highly Accurate Dichotomous Image Segmentation research](https://github.com/xuebinqin/DIS)." (https://huggingface.co/schirrmacher/ormbg) |
| InSPyReNet | for the transparent-background checkpoints: not stated; the README: "We trained our model on [Dichotomous Image Segmentation dataset (DIS5K)]" for a separate checkpoint |
| MODNet | "All the models behind the following demos are trained on the datasets mentioned in [our paper]" |
| withoutBG Open Weights | not stated |
| BRIA RMBG-1.4 | "Bria-RMBG model was trained with over 12,000 high-quality, high-resolution, manually labeled (pixel-wise accuracy), fully licensed images." |
| BRIA RMBG-2.0 | not stated in the card fetched (the model is gated) |

So every general-purpose candidate except BRIA's names DIS5K or builds on DIS work: IS-Net, BiRefNet, BEN2 (with its own data), InSPyReNet,
and ormbg ("based on" DIS). DIS5K's terms of use are a PDF in the DIS repository
(https://github.com/xuebinqin/DIS/blob/main/DIS5K-Dataset-Terms-of-Use.pdf, checked 2026-11-09), quoted:

- "2. Intended use: The Dataset is available for non-commercial use in research or educational purpose. As long as you use The
  Dataset for these purposes, you can edit or process images and annotations in this database. Without permission from the original
  authors, commercial use of this dataset is prohibited even after copying, editing, processing or any operations of this database.
  Please contact us for commercial use or if you are uncertain about the decision."
- "4. Distribution: It is prohibited to sell, transfer, lend, lease, resell, distribute, etc., as it is, or copy, edit, or process this
  database, in whole or in part."
- "The original authors of The Dataset may arbitrarily revise this agreement without the approval of the registrant and this data user."

Whether weights trained on the dataset count as "processing" it is not stated in these terms or in any card; it is the open question
for every model above that names DIS5K, including BEN2 and BiRefNet despite their MIT tags. (A reading, not legal advice.)

## Each model

### IS-Net general-use (DIS), the model 0035 is built with
- Code: Apache-2.0. https://github.com/xuebinqin/DIS (`LICENSE.md`: "Apache License / Version 2.0, January 2004"). README: "Our code
  and evaluation metric use Apache License 2.0. The Terms of use for our DIS5K dataset is provided as
  [DIS5K-Dataset-Terms-of-Use.pdf](DIS5K-Dataset-Terms-of-Use.pdf)."
- Weights: not stated. The README only gives the download: "The optimized model for general use of our IS-Net is now released:
  ```isnet-general-use.pth``` (for general use, this is NOT DIS V2.0.) from [(Google Drive)](…)".
- ONNX: 178.6 MB (`isnet-general-use.onnx`, rembg's conversion; rembg's own code is MIT, "Licensed under the [MIT License]", which
  says nothing of the weights it converts).
- Cost: ADR-0006's row above; in the app, T006 of 0035: 1.2 to 1.4 s at 1920 x 1080, 1.2 to 2.0 s at 8000 x 8000, at most 1.17 GB.

### u2netp and silueta (U-2-Net)
- Code: Apache-2.0. https://github.com/xuebinqin/U-2-Net (`LICENSE`: "Apache License / Version 2.0, January 2004").
- Weights: not stated (the README links the `.pth` files on Google Drive and says nothing of their licence).
- silueta is not a new training: its author, issue https://github.com/xuebinqin/U-2-Net/issues/295 (2022-03-30): "For this version I
  didn't retrain it, I used the pretrained model and quantized it using the quantization utils from onnx-runtime". Its terms are
  therefore U-2-Net's weights' terms: not stated.
- ONNX: u2netp 4.6 MB, silueta 44.2 MB (rembg's release files).

### BiRefNet and BiRefNet-lite
- Code: MIT. https://github.com/ZhengPeng7/BiRefNet (`LICENSE`: "MIT License / Copyright (c) 2024 ZhengPeng").
- Weights: the model cards https://huggingface.co/ZhengPeng7/BiRefNet_lite and https://huggingface.co/ZhengPeng7/BiRefNet declare
  "license: mit" (Hugging Face tag `license:mit`); the ONNX conversions https://huggingface.co/onnx-community/BiRefNet_lite-ONNX and
  https://huggingface.co/onnx-community/BiRefNet-ONNX declare the same.
- ONNX: lite 224 MB (rembg's `BiRefNet-general-bb_swin_v1_tiny-epoch_232.onnx`); full 1.04 GB (`birefnet-general.onnx` in withoutBG's
  repository).
- Cost: the lite one, ADR-0006: 9.3 / 5.9 s and 12 GB peak at 1920 x 1080: ruled out by the 2 GB budget.

### BEN2 Base (Prama)
- Code and weights: https://huggingface.co/PramaLLC/BEN2, front matter "license: mit". The card also says that access to the
  commercial model is by contacting the vendor, BackgroundErase (https://backgrounderase.com) (the open one is "Base"; a separate commercial model exists).
- ONNX: `BEN2_Base.onnx`, 222.9 MB (Hugging Face API).
- Cost: not measured.

### ormbg (Open Remove Background Model)
- Card: https://huggingface.co/schirrmacher/ormbg and https://github.com/schirrmacher/ormbg, front matter "license: apache-2.0"; no
  `LICENSE` file found at the repository's root (main branch).
- Origin: "This model is a **fully open-source background remover** optimized for images with humans. It is based on [Highly Accurate
  Dichotomous Image Segmentation research](https://github.com/xuebinqin/DIS)." The repository also holds `models/isnet-base-model.pth`
  (176.6 MB): whether ormbg's weights start from IS-Net's (whose own terms are not stated) is not stated.
- ONNX: `ormbg.onnx`, 176.2 MB. Trained for people ("optimized for images with humans").
- Cost: not measured.

### InSPyReNet (transparent-background)
- Code: MIT. https://github.com/plemeri/InSPyReNet (`LICENSE`: "MIT License / Copyright (c) 2021 Taehun Kim") and
  https://github.com/plemeri/transparent-background (`LICENSE`: "MIT License / Copyright (c) 2022 Taehun Kim").
- Weights: not stated. They are release files of transparent-background (`ckpt_base.pth`, `ckpt_fast.pth`, `ckpt_base_nightly.pth`,
  367.5 MB each, release 1.2.12); the README says it was also "trained our model on [Dichotomous Image Segmentation dataset (DIS5K)]".
- ONNX: none published by the authors.

### MODNet (portrait matting)
- https://github.com/ZHKKKe/MODNet (`LICENSE`: Apache-2.0). README: "The code, models, and demos in this repository (excluding GIF
  files under the folder `doc/gif`) are released under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) license."
- For portraits only (people), not "every main subject" as REQ-0035-03 asks.

### withoutBG Open Weights
- SDK: Apache-2.0 (https://github.com/withoutbg/withoutbg, README: "This Python SDK is licensed under Apache License 2.0.").
- Weights: https://huggingface.co/withoutbg/withoutbg-openweights-onnx, `LICENSE`: "The withoutBG Open Weights model in this
  repository (the ONNX graphs withoutbg-open-weights-backbone.onnx, withoutbg-open-weights.onnx and birefnet-general.onnx, and the
  sidecar withoutbg-open-weights.onnx.json) is distributed under the withoutBG Open Weights license: https://withoutbg.com/open-model/license"
  and "The model is built from third-party components that remain under their own licenses, and your use must also comply with them:
  - DINOv3 (Meta Platforms, Inc.): DINOv3 License … Built with DINOv3. - Depth Anything V2 (small): Apache License 2.0 - BiRefNet
  (ZhengPeng7): MIT License". Commercial licensing is by contacting the vendor, withoutBG (https://withoutbg.com).
- The licence page https://withoutbg.com/open-model/license: SPDX "Apache-2.0 AND LicenseRef-DINOv3"; "Provide a copy of the DINOv3
  License when you distribute the Model or derivatives to third parties."; "Display "Built with DINOv3" prominently" (an "app About
  screen" among the places named).
- Meta's DINOv3 License https://ai.meta.com/resources/models-and-libraries/dinov3-license/: "If you distribute or make the DINO
  Materials, or any derivative works thereof, available to a third party, you may only do so under the terms of this Agreement."; its
  restrictions include "military or warfare purposes, nuclear industries or applications, espionage"; "Meta may terminate this
  Agreement if you are in breach of any term or condition of this Agreement."
- ONNX: 104.1 MB + the backbone 350.4 MB (+ a 1.04 GB BiRefNet branch for hard objects).
- Reading: the DINOv3 terms travel with every copy and restrict use, which GPL-3.0 does not allow for the program itself (its section
  10: "You may not impose any further restrictions on the exercise of the rights granted or affirmed under this License"); whether a
  weights file loaded as data is part of the GPL-covered work is not settled, so shipping it is doubtful, and Debian main (DFSG) would
  refuse use restrictions in any case.

### BRIA RMBG-1.4 and RMBG-2.0
- RMBG-1.4: https://huggingface.co/briaai/RMBG-1.4, front matter "license: other", "license_name: bria-rmbg-1.4", "license_link:
  https://bria.ai/bria-huggingface-model-license-agreement/"; card: "RMBG v1.4 is available as a source-available model for
  non-commercial use."; "To purchase a commercial license, simply click [Here]".
- RMBG-2.0: https://huggingface.co/briaai/RMBG-2.0, front matter "license: other", "license_name: bria-rmbg-2.0", "license_link:
  https://creativecommons.org/licenses/by-nc/4.0/deed.en"; gated ("auto").
- Reading: non-commercial terms cannot ship in a GPL-3.0 app, which lets anyone use it commercially.

## What the table leaves open
- None of the clearly permissive weights (BiRefNet MIT, BEN2 MIT, ormbg Apache-2.0, MODNet Apache-2.0) has been run in Easel or on
  0035's test set; BEN2 and ormbg are about IS-Net's size, BiRefNet's are far over the 2 GB budget, MODNet is for people only.
- Dataset terms (DIS5K for IS-Net, InSPyReNet and perhaps ormbg) are a separate question from the weights' licence; none of the cards
  above says how its training data's terms bear on the weights.
