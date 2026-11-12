libm = meson.get_compiler('c').find_library('m', required: false)

# Core: libc only (no GLib, GTK or Cairo), so it is built without those dependencies.
easel_core_inc = include_directories('core')
easel_core = static_library('easel-core',
  'core/change-log.c',
  'core/cursor-shape.c',
  'core/fill.c',
  'core/frame-time.c',
  'core/catalog.c',
  'core/print-layout.c',
  'core/figure.c',
  'core/curve-bend.c',
  'core/black-white.c',
  'core/remove-bg.c',
  'core/footprint.c',
  'core/magnifier.c',
  'core/format.c',
  'core/jpeg-exif.c',
  'core/cover.c',
  'core/history.c',
  'core/gradient.c',
  'core/selection.c',
  'core/shapes.c',
  'core/grid.c',
  'core/image.c',
  'core/lasso.c',
  'core/lattice.c',
  'core/line.c',
  'core/palette.c',
  'core/ini.c',
  'core/number.c',
  'core/settings.c',
  'core/rich-text.c',
  'core/stroke.c',
  'core/text-buffer.c',
  'core/stretch-skew.c',
  'core/text-put.c',
  'core/transform.c',
  'core/view.c',
  include_directories: easel_core_inc,
  dependencies: libm,
)
easel_core_dep = declare_dependency(link_with: easel_core, include_directories: easel_core_inc,
  dependencies: libm)

# io: GLib, GIO and gdk-pixbuf; never needs a display.
gio_dep = dependency('gio-2.0')
pixbuf_dep = dependency('gdk-pixbuf-2.0')
easel_io_inc = include_directories('io')
easel_io = static_library('easel-io',
  'io/palette-file.c',
  'io/picture-file.c',
  'io/settings-file.c',
  include_directories: easel_io_inc,
  dependencies: [easel_core_dep, gio_dep, pixbuf_dep],
)
easel_io_dep = declare_dependency(link_with: easel_io, include_directories: easel_io_inc,
  dependencies: [easel_core_dep, gio_dep, pixbuf_dep])

ui_conf = configuration_data()
ui_conf.set_quoted('APP_ID', app_id)
ui_conf.set_quoted('RESOURCE_PATH', resource_path)
ui_conf.set_quoted('VERSION', meson.project_version())
ui_conf.set_quoted('LOCALE_DIR', get_option('prefix') / get_option('localedir'))
ui_conf.set_quoted('HELP_PAGE', get_option('prefix') / get_option('datadir') / 'easel' / 'help' / 'index.html')
configure_file(output: 'config.h', configuration: ui_conf)

gnome = import('gnome')
ui_resources = gnome.compile_resources('easel-resources', 'ui/easel.gresource.xml',
  source_dir: 'ui',
  c_name: 'easel',
)

easel_ui_inc = include_directories('.', 'ui')

easel_ui = static_library('easel-ui',
  'ui/application.c',
  'ui/attributes-dialog.c',
  'ui/stretch-skew-dialog.c',
  'ui/black-white-dialog.c',
  'ui/remove-bg-card.c',
  'ui/help.c',
  'ui/i18n.c',
  'ui/preferences-dialog.c',
  'ui/print.c',
  'ui/unit-row.c',
  'ui/canvas.c',
  'ui/clipboard.c',
  'ui/text-render.c',
  'ui/thumbnail-card.c',
  'ui/grid-dialog.c',
  'ui/palette-bar.c',
  'ui/font-card.c',
  'ui/status-bar.c',
  'ui/tool-options.c',
  'ui/tool.c',
  'ui/window.c',
  ui_resources,
  include_directories: easel_ui_inc,
  dependencies: [gtk_dep, adw_dep, easel_core_dep, easel_io_dep],
)

easel_ui_dep = declare_dependency(
  link_whole: easel_ui,
  include_directories: easel_ui_inc,
  dependencies: [gtk_dep, adw_dep, easel_core_dep, easel_io_dep],
)

executable('easel', 'ui/main.c',
  dependencies: easel_ui_dep,
  install: true,
)
