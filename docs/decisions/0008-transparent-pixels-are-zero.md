# 0008 A fully transparent pixel is 0x00000000

**Status:** Accepted (2026-11-10, JOD)

**Context:**
Pixels are straight alpha (ADR-0002), so a pixel with alpha 0 still carries red, green and blue. Remove Background kept each removed
pixel's color under alpha 0 (REQ-0035-07: "alpha 0 with its color channels kept"), so a removed backdrop with noise held a hundred
hidden colors (JOD's candle picture: 103 distinct values among 292 885 removed pixels). Fill compares all four channels exactly
(REQ-0007-05), so it filled only the patches of one hidden color: an area that looks like one transparent color behaved like many.
The same happened wherever a transparent pixel was made with a color left in it: Invert Colors and Black and White on transparent
pixels, a tool drawing with a fully transparent color, the gradient's fade, a file or a paste whose transparent pixels hold colors.

**Decision:**
Every operation that produces a fully transparent pixel writes 0x00000000 (RGBA 0, 0, 0, 0): every tool, command and transform, the
gradient and Remove Background. Fill treats every fully transparent pixel as one color. Invert Colors and Black and White leave a
fully transparent pixel untouched. Nothing is added at opening or saving: saving writes the pixels as they are, and opening already reads a fully
transparent pixel as 0 (the decoder of feature 0005 has done so since 2026-10-15, found while making this decision). A picture
that still holds a hidden color under alpha 0 (an older undo step, a pixel copied from one) keeps it until it is edited, and Fill's
rule covers it. Partly transparent pixels keep their color (ADR-0002 stands for them).

**Consequences:**
- P7 gains a point; REQ-0035-07's "alpha 0 with its color channels kept" becomes "0x00000000".
- REQ-0001-30's exact PNG round trip is unchanged (JOD, 2026-11-10: no normalising added at open or save).
- Fill keeps Undo exact over fully transparent pixels of several hidden colors by logging each pixel's old value; a fill over
  pixels of one value keeps its compact step.
- An undo restores the pixels as they were, so a step taken before this rule may hold a hidden color until it is drawn over; Fill's
  rule covers it.
- Remove Background: pieces still bring a removed pixel back from the picture as it was before the card, not from the removed value.
