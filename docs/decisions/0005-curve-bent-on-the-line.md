# 0005 Curve bends where the user drags, not as XP

**Status:** Accepted (2026-11-09, with spec 0033; JOD decided the behavior on 2026-11-07 and 2026-11-09)

**Context:**
P1 asks for XP Paint's functionality and workflow, and changing a principle needs an ADR. XP's Curve bends the line by a click
anywhere, and its second click puts the curve down (0011, REQ-0011-04): the bend lands away from where the user pointed and cannot be
tried again. JOD wants the line taken where it should bend, as drawing programs do (`docs/phase2-plan.md` E).

**Decision:**
A named exception to P1 for Curve only: after the line, the user drags points of the curve to bend it, each bent point staying where
it is dropped, at most two bends, the curve reshaped until a press away from it, Enter or a put-down finishes it (spec 0033). No
option brings back XP's way. `docs/product.md` names the exception next to "XP-faithful by default".

**Consequences:**
0011's three-stage curve, its two-click scenarios and its ui tests change with 0033. The other tools keep XP's workflow; a further
exception needs its own ADR.
