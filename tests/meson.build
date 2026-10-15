foreach name : ['history', 'image', 'number', 'palette', 'savename', 'settings', 'stroke', 'view']
  test('core-' + name,
    executable('test-' + name, 'core/test-' + name + '.c', dependencies: easel_core_dep),
    suite: 'core',
  )
endforeach

foreach name : ['palette-file', 'png', 'settings-file']
  test('io-' + name,
    executable('test-' + name, 'io/test-' + name + '.c', dependencies: easel_io_dep),
    suite: 'io',
  )
endforeach

ui_env = environment()
ui_env.set('GSK_RENDERER', 'cairo')
ui_env.set('GTK_A11Y', 'none')
# File dialogs open in-process, so tests can find and close them.
ui_env.set('GDK_DEBUG', 'no-portals')

test_window = executable('test-window', 'ui/test-window.c',
  dependencies: easel_ui_dep,
)
test('ui-window', test_window, env: ui_env, suite: 'ui')

# Performance budgets (docs/engineering.md): meson test -C build --benchmark
perf_env = environment()
perf_env.set('GTK_A11Y', 'none')
benchmark('perf-0001', executable('perf-0001', 'perf/perf-0001.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 120)
benchmark('perf-0002', executable('perf-0002', 'perf/perf-0002.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 120)
benchmark('perf-0004', executable('perf-0004', 'perf/perf-0004.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 180)
