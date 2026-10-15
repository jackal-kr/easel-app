# 0002 Straight-alpha pixels

**Status:** Accepted (2026-10-15)

**Context:**
The constitution suggested `uint32_t` 0xAARRGGBB premultiplied pixels and asked the first plan to revisit this.
Premultiplied storage matches Cairo and GDK, so uploading to the screen is a plain copy, with conversion only when a file
is saved or loaded. But it loses color in partly transparent pixels. Feature 0001 requires an exact PNG round trip
including transparency (REQ-0001-30) and allows any `#RRGGBBAA` background.

**Decision:**
Store pixels as `uint32_t` 0xAARRGGBB with straight (non-premultiplied) alpha. Convert only at the edges that need
premultiplied data.

**Consequences:**
- Straight alpha keeps exactly the color the user picked. A PNG round trip is lossless even for semi-transparent pixels.
- Operations such as invert stay simple (255 - channel), and pixel tests compare plain values.
- Display: the texture is declared `GDK_MEMORY_B8G8R8A8` on little-endian systems (`GDK_MEMORY_A8R8G8B8` on big-endian).
  GDK still converts to premultiplied when drawing; we just don't write that code.
- Cairo output (text, feature 0011) is premultiplied and needs one conversion into the picture.
- Blending (gradient, transparent paste) needs a division by alpha. These are a few core routines, testable headless.
- The constitution's Technical section states the straight-alpha rule.
