---
paths:
  - "src/ui/**"
  - "data/**"
---
- Widgets translate input into core calls; no drawing logic here.
- Follow `docs/design/ui-ux.md` and `docs/design/interaction-model.md`; GNOME HIG, libadwaita widgets only.
- Pending state (floating selection, text, unfinished curve) must be committed or ask before save, close, New, tool change.
- Strings translatable, one place per dialog, no concatenation.
