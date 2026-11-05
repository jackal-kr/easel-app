---
paths:
  - "src/core/**"
---
- libc only: no GTK, GLib or Cairo includes. No global state.
- Every function here is covered by a headless test. Bounds-clip every pixel access; check size arithmetic for overflow.
- Follow `docs/engineering.md`; zero warnings.
