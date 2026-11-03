# These tests fail an allocation of the core on purpose.
core_wrap = {
  'stretch-skew': ['-Wl,--wrap=malloc', '-Wl,--wrap=calloc', '-Wl,--wrap=realloc'],
  'settings-edit': ['-Wl,--wrap=malloc'],
}
foreach name : ['cursor', 'stretch-skew', 'settings-edit', 'window-extras', 'print-layout', 'figure', 'figure-apply', 'figure-curve', 'figure-polygon', 'figure-step', 'gradient', 'selection', 'fill', 'footprint', 'format', 'grid', 'history', 'image', 'lasso', 'lattice', 'magnifier', 'number', 'palette', 'resize', 'settings', 'spray', 'stroke', 'rich-text', 'text-buffer', 'text-put', 'transform', 'view']
  test('core-' + name,
    executable('test-' + name, 'core/test-' + name + '.c', dependencies: easel_core_dep,
               link_args: core_wrap.get(name, [])),
    env: {'EASEL_TEST_DATA': meson.current_source_dir() / 'data'},
    suite: 'core',
  )
endforeach

foreach name : ['palette-file', 'picture-file', 'png', 'settings-file', 'settings-change']
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
ui_env.set('EASEL_HELP_PAGE', help_page)
# File dialogs open in-process, so tests can find and close them.
ui_env.set('GDK_DEBUG', 'no-portals')

test_window = executable('test-window', 'ui/test-window.c', 'ui/memory-clipboard.c',
  dependencies: [easel_ui_dep, dependency('gio-unix-2.0')],
)
# One meson test per feature, each running the cases whose GTest path starts with the feature number (the cases are
# registered as /0001/..., /0002/..., so every case is in exactly one of them); see docs/engineering.md, Testing.
# Not parallel: run in parallel, the ui windows failed as5_zoom_keeps_center with a stale layout (598.25 against 1196.5)
# in 2 of 3 runs. Earlier one-off failures of the ui tests, run as a single executable, are unexplained.
foreach feature : ['0001', '0002', '0003', '0004', '0005', '0006', '0007', '0008', '0009', '0010', '0011', '0012', '0013', '0014', '0015', '0016', '0017', '0018', '0019', '0020', '0021', '0025', '0026']
  test('ui-' + feature, test_window, args: ['-p', '/' + feature], env: ui_env, suite: 'ui', timeout: 180,
       is_parallel: false)
endforeach
# What 0025 asks of the picture's squares, under the renderer of a real session as well (under another where it has none).
ui_vulkan_env = environment()
ui_vulkan_env.set('GSK_RENDERER', 'vulkan')
ui_vulkan_env.set('GTK_A11Y', 'none')
ui_vulkan_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
ui_vulkan_env.set('GDK_DEBUG', 'no-portals')
test('ui-0025-vulkan', test_window, args: ['-p', '/0025/as1_squares_at_whole_zooms', '-p', '/0025/as1c_edges_at_fractional_zoom', '-p', '/0025/as1d_grid_on_cell_edges', '-p', '/0025/as2c_pointer_hits_the_drawn_pixel'],
     env: ui_vulkan_env, suite: 'ui', timeout: 180, is_parallel: false)
# AS-1b of 0025 needs a screen of scale 2 and skips elsewhere: tools/isolated.sh --scale 2 meson test -C build ui-0025-scale2
test('ui-0025-scale2', test_window, args: ['-p', '/0025-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0019-scale2', test_window, args: ['-p', '/0019-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0021-scale2', test_window, args: ['-p', '/0021-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
# The cases that take more than about 15 s each (registered under /long/): run by the gate and /sdd-check, not by tools/mutate.sh,
# which names the suites it runs (a mutant that only these can kill is listed in tests/mutants.txt with test: ui-0014-long).
test('ui-0014-long', test_window, args: ['-p', '/long/0014'], env: ui_env, suite: ['ui', 'long'], timeout: 400, is_parallel: false)

# The GDK clipboard behaviour 0016 relies on, probed by hand in the real session (the isolated session holds no clipboard): built, not
# a test; run build/tests/probe-clipboard with the pointer outside its window (Plan of 0016, "The clipboard seam").
executable('probe-clipboard', 'probe/probe-clipboard.c', dependencies: easel_ui_dep)
# A clipboard owner that answers late or never, for the manual step M2 of 0016 (REQ-0016-15): build/tests/slow-clipboard-owner [SECONDS | never].
executable('slow-clipboard-owner', 'probe/slow-clipboard-owner.c', dependencies: easel_ui_dep)
# Two processes, as Easel and another app are: `own` puts Easel's copy on the clipboard, `read` reads it back (M1 of 0016).
executable('probe-clipboard-xproc', 'probe/probe-clipboard-xproc.c', dependencies: easel_ui_dep)
# Feature 0017's risks R1 to R3: how Pango and Cairo draw text into an A8 surface at picture scale (no display needed).
executable('probe-text-render', 'probe/probe-text-render.c', dependencies: easel_ui_dep)
executable('probe-text-hinting', 'probe/probe-text-hinting.c', dependencies: easel_ui_dep)
# Feature 0025, T001: the picture built at device size against the renderer's scaling, at a fractional zoom: edges under any renderer
# (probe-edge-drawing edges, through tools/isolated.sh) and the cost of a frame (probe-edge-drawing frames, in the real session).
executable('probe-edge-drawing', 'probe/probe-edge-drawing.c', dependencies: easel_ui_dep)
# Feature 0018, T001: the memory peak and time of each command on the whole picture, one command a process: probe-transform-memory rot90 8000.
executable('probe-transform-memory', 'probe/probe-transform-memory.c', dependencies: easel_core_dep)
# Feature 0017's risk R6: what Writer and Firefox do with rich text on the clipboard: `own` (a click) and `read NAME`.
executable('probe-clipboard-rich', 'probe/probe-clipboard-rich.c', dependencies: easel_ui_dep,
  c_args: ['-DEASEL_SOURCE_DIR="' + meson.project_source_root() + '"'])
executable('probe-paste-text', 'probe/probe-paste-text.c', dependencies: easel_ui_dep,
  c_args: ['-DEASEL_SOURCE_DIR="' + meson.project_source_root() + '"'])

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
benchmark('perf-0011', executable('perf-0011', 'perf/perf-0011.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0012', executable('perf-0012', 'perf/perf-0012.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0013', executable('perf-0013', 'perf/perf-0013.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0014', executable('perf-0014', 'perf/perf-0014.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 900)
benchmark('perf-0015', executable('perf-0015', 'perf/perf-0015.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 900)
benchmark('perf-0016', executable('perf-0016', 'perf/perf-0016.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0017', executable('perf-0017', 'perf/perf-0017.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 600)
benchmark('perf-0018', executable('perf-0018', 'perf/perf-0018.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 900)
benchmark('perf-0019', executable('perf-0019', 'perf/perf-0019.c', dependencies: easel_core_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0008', executable('perf-0008', 'perf/perf-0008.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 600)
benchmark('perf-0025', executable('perf-0025', 'perf/perf-0025.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 600)

