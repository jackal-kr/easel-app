# Coding standards (C)

- GNU-ish style: 2-space indent, space before `(`, return type on its own line, `/* */` comments (avoid `//`).
  Match the surrounding file; do not reformat untouched code.
- Warnings are bugs: build with `-Wshadow -Wmissing-prototypes` and friends, zero warnings. Non-static functions have prototypes.
- Naming: public UI API `easel_<module>_`; internals shared inside one module share a short prefix.
- `src/core`: libc only. No GTK, GLib or Cairo includes. No global state; functions take the object they act on.
- `src/ui`: turns input into core calls; no drawing logic in widgets. GLib/GTK lifetime: `g_autoptr`, disconnect or tie signal
  handlers to the object, unparent popovers in dispose.
- Every pixel write goes through one dirty-marking path (defined in the plan of the feature that introduces it).
- Menus and actions: declared in data files, registered in one place; an absent action disables its item.
- Strings: one place per dialog, no sentence concatenation.
- Comments explain why, not what. No dead code, no commented-out code.
