foreach name : ['cursor', 'figure', 'figure-apply', 'fill', 'footprint', 'format', 'grid', 'history', 'image', 'magnifier', 'number', 'palette', 'resize', 'settings', 'spray', 'stroke', 'view']
  test('core-' + name,
    executable('test-' + name, 'core/test-' + name + '.c', dependencies: easel_core_dep),
    suite: 'core',
  )
endforeach

foreach name : ['palette-file', 'picture-file', 'png', 'settings-file']
  test('io-' + name,
    executable('test-' + name, 'io/test-' + name + '.c', dependencies: easel_io_dep),
    env: {'EASEL_TEST_DATA': meson.current_source_dir() / 'data'},
    suite: 'io',
  )
endforeach

ui_env = environment()
ui_env.set('GSK_RENDERER', 'cairo')
ui_env.set('GTK_A11Y', 'none')
ui_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
# File dialogs open in-process, so tests can find and close them.
ui_env.set('GDK_DEBUG', 'no-portals')

test_window = executable('test-window', 'ui/test-window.c',
  dependencies: easel_ui_dep,
)
# One meson test per feature, each running the cases whose GTest path starts with the feature number (the cases are
# registered as /0001/..., /0002/..., so every case is in exactly one of them); see docs/engineering.md, Testing.
# Not parallel: run in parallel, the ui windows failed as5_zoom_keeps_center with a stale layout (598.25 against 1196.5)
# in 2 of 3 runs. Earlier one-off failures of the ui tests, run as a single executable, are unexplained.
foreach feature : ['0001', '0002', '0003', '0004', '0005', '0006', '0007', '0008', '0009', '0010']
  test('ui-' + feature, test_window, args: ['-p', '/' + feature], env: ui_env, suite: 'ui', timeout: 180,
       is_parallel: false)
endforeach

# Performance budgets (docs/engineering.md): meson test -C build --benchmark
perf_env = environment()
perf_env.set('GTK_A11Y', 'none')
benchmark('perf-0001', executable('perf-0001', 'perf/perf-0001.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 120)
benchmark('perf-0002', executable('perf-0002', 'perf/perf-0002.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 120)
benchmark('perf-0004', executable('perf-0004', 'perf/perf-0004.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 180)
benchmark('perf-0005', executable('perf-0005', 'perf/perf-0005.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0006', executable('perf-0006', 'perf/perf-0006.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0007', executable('perf-0007', 'perf/perf-0007.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0010', executable('perf-0010', 'perf/perf-0010.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
