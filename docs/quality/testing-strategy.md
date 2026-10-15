# Testing strategy

- **Every scenario (AS-n) is verified** by an automated test or a numbered manual check named in the spec.
  Tests are named after scenarios (`as3_undo_restores_buffer`) so `sdd-analyze` can trace them.
- **Layer 1, core (headless, libc only):** pixels, history, transforms, clipping. Fast, exact expectations, property checks
  (undo restores the original buffer, transform then inverse is identity).
- **Layer 2, widget behavior:** synthetic press/drag/release through the canvas input entry points; asserts pixels, undo/redo
  and modified state. Needs a display: skip with exit code 77 otherwise; run headless with `GSK_RENDERER=cairo`.
- **Layer 3, manual:** short numbered checklists at each checkpoint in `tasks.md` for look and feel. Say plainly what cannot be tested headless.
- **Test first** where feasible (`test-writer` agent); a bug fix starts with a failing test.
- **Gates:** build warning-free, all tests green after each task; `tools/check-trace.sh` clean before a spec is accepted.
- **Later:** CI in a container or the Flatpak SDK (libadwaita >= 1.8 needed); screenshot checks only if they prove worth the upkeep.
