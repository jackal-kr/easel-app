help_page = meson.current_source_dir() / 'help' / 'index.html'
install_data('help/index.html', install_dir: get_option('datadir') / 'easel' / 'help')

# An uninstalled build finds this copy from its program's folder, however it was started (0021).
configure_file(input: 'help/index.html', output: 'help-index.html', copy: true)

# Each language's page, its copy for an uninstalled build, and where it goes (REQ-0023-09).
foreach code : ['cs', 'de', 'es', 'fr', 'hu', 'it', 'pl', 'pt', 'ro', 'ru', 'uk']
  install_data('help' / code / 'index.html', install_dir: get_option('datadir') / 'easel' / 'help' / code)
  configure_file(input: 'help' / code / 'index.html', output: 'help-' + code + '.html', copy: true)
endforeach

# What the desktop needs (0024): the entry and the software-center metadata, with their translations, and the icons.
i18n_data = import('i18n')
desktop_file = i18n_data.merge_file(
  input: app_id + '.desktop.in', output: app_id + '.desktop', type: 'desktop', po_dir: '../po',
  install: true, install_dir: get_option('datadir') / 'applications')
metainfo_file = i18n_data.merge_file(
  input: app_id + '.metainfo.xml.in', output: app_id + '.metainfo.xml', po_dir: '../po',
  install: true, install_dir: get_option('datadir') / 'metainfo')
install_data('icons/hicolor/scalable/apps' / (app_id + '.svg'), install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps')
install_data('icons/hicolor/symbolic/apps' / (app_id + '-symbolic.svg'), install_dir: get_option('datadir') / 'icons/hicolor/symbolic/apps')
