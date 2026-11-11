project('easel', 'c',
  version: '0.1.0-beta.1',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 1.0',
  default_options: ['c_std=gnu11', 'warning_level=2', 'buildtype=debugoptimized'],
)

app_id = 'io.github.jackal_kr.Easel'
# GResource base path; GTK finds the app's icons under it, so it must follow the app ID.
resource_path = '/' + app_id.replace('.', '/')

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments([
  '-Wshadow',
  '-Wmissing-prototypes',
  '-Wstrict-prototypes',
  '-Wformat=2',
  '-Wundef',
  '-Wno-unused-parameter',
  '-Wno-missing-field-initializers',
]), language: 'c')

gtk_dep = dependency('gtk4', version: '>= 4.16')
adw_dep = dependency('libadwaita-1', version: '>= 1.8')


subdir('po')
subdir('data')
subdir('src')
subdir('tests')
