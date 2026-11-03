help_page = meson.current_source_dir() / 'help' / 'index.html'
install_data('help/index.html', install_dir: get_option('datadir') / 'easel' / 'help')

# An uninstalled build finds this copy from its program's folder, however it was started (0021).
configure_file(input: 'help/index.html', output: 'help-index.html', copy: true)
