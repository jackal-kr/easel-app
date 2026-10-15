# Architecture

Written during the first plan (feature 0001) and updated by every plan that changes it. Until then, only the
constraints in `docs/constitution.md` apply: `src/core` pure C and headless-testable; `src/ui` thin; one source of
truth for tool definitions; no threads or caches without a measurement.

Sections to fill: layers and modules, build layout, data flow for an edit (input -> core -> render), undo model,
settings, packaging layout.
