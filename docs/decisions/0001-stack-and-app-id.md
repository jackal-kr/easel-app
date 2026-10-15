# 0001 Stack and app ID

**Status:** Accepted

**Context:**

Easel is a native GNOME desktop application inspired by the simplicity and functionality of Windows XP MS Paint. It should provide a responsive, lightweight user experience while using the GNOME platform naturally.

The project needs a compiled implementation, a native GTK interface, a suitable 2D drawing library, and a build system that supports reliable development and testing.

The core document model and drawing operations must remain testable without initializing a graphical display.

**Decision:**

Use the following technology stack:

- **C** - application and core implementation language.
- **GTK 4** - widget toolkit, input handling, layout, and UI integration.
- **libadwaita >= 1.8** - native GNOME widgets and visual conventions.
- **Cairo** - CPU-based 2D drawing where appropriate.
- **GLib / GObject / GIO** - foundational utilities, object model, event loop, file operations, actions, and platform integration.
- **GDK / GSK** - display integration, input, textures, and GTK rendering infrastructure.
- **Pango** - text layout and font handling.
- **gdk-pixbuf** - image loading and saving where supported by available loaders.
- **gettext** - application internationalization.
- **Meson / Ninja** - build configuration and execution.
- **GPL-3.0-or-later** - project license.

Use `io.github.jackal_kr.EaselSdd` as the development application ID. Keep it distinct from other builds of Easel; the application ID and display name may change before release.

Keep the core document model and drawing operations independent of GTK widgets so they can be tested headlessly.

Treat the libraries above as architectural building blocks, not as a requirement to use every library directly in every module. Choose rendering and image-processing paths according to the needs of the relevant features.

**Consequences:**

- The application uses a compiled C implementation and the native GNOME platform stack.
- The core can be tested without a running graphical session.
- GTK and libadwaita provide the UI layer; they do not own the authoritative document state.
- Cairo is available for CPU-based drawing but is not mandated as the renderer for every operation.
- Image-format support depends on the implemented functionality and available library loaders.
- Changing the language, minimum library versions, build system, or major architectural boundaries requires a new or amended ADR.
- Feature specifications remain focused on user-visible behavior rather than unnecessarily prescribing implementation details.
