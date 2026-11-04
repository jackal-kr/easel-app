help_page = meson.current_source_dir() / 'help' / 'index.html'
install_data('help/index.html', install_dir: get_option('datadir') / 'easel' / 'help')

# An uninstalled build finds this copy from its program's folder, however it was started (0021).
configure_file(input: 'help/index.html', output: 'help-index.html', copy: true)

# Each language's page, its copy for an uninstalled build, and where it goes (REQ-0023-09).
foreach code : ['cs', 'de', 'es', 'fr', 'hu', 'it', 'pl', 'pt', 'ro', 'ru', 'uk']
  install_data('help' / code / 'index.html', install_dir: get_option('datadir') / 'easel' / 'help' / code)
  configure_file(input: 'help' / code / 'index.html', output: 'help-' + code + '.html', copy: true)
endforeach
