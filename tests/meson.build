# These tests fail an allocation of the core on purpose.
core_wrap = {
  'stretch-skew': ['-Wl,--wrap=malloc', '-Wl,--wrap=calloc', '-Wl,--wrap=realloc'],
  'settings-edit': ['-Wl,--wrap=malloc'],
}
foreach name : ['cursor', 'stretch-skew', 'settings-edit', 'window-extras', 'print-layout', 'catalog', 'figure', 'figure-apply', 'figure-curve', 'figure-polygon', 'figure-step', 'cover', 'stroke-smooth', 'curve-bend', 'black-white', 'remove-bg', 'gradient', 'selection', 'fill', 'footprint', 'format', 'jpeg-exif', 'grid', 'history', 'image', 'lasso', 'lattice', 'magnifier', 'number', 'palette', 'resize', 'settings', 'spray', 'stroke', 'rich-text', 'text-buffer', 'text-put', 'transform', 'view']
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
ui_env.set('EASEL_TEST_LANGUAGE', 'en')    # the suite compares English (AS-13 of 0023)
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
foreach feature : ['0001', '0002', '0003', '0004', '0005', '0006', '0007', '0008', '0009', '0010', '0011', '0012', '0013', '0014', '0015', '0016', '0017', '0018', '0019', '0020', '0021', '0025', '0026', '0023']
  test('ui-' + feature, test_window, args: ['-p', '/' + feature], env: ui_env, suite: 'ui', timeout: 180,
       is_parallel: false)
endforeach
# What 0025 asks of the picture's squares, under the renderer of a real session as well (under another where it has none).
# 0022 reads names and states: GTK makes no accessible context under GTK_A11Y=none.
ui_a11y_env = environment()
ui_a11y_env.set('GSK_RENDERER', 'cairo')
ui_a11y_env.set('GTK_A11Y', 'test')
ui_a11y_env.set('EASEL_TEST_LANGUAGE', 'en')
ui_a11y_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
ui_a11y_env.set('EASEL_HELP_PAGE', help_page)
ui_a11y_env.set('GDK_DEBUG', 'no-portals')
test('ui-0022', test_window, args: ['-p', '/0022'], env: ui_a11y_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0039-a11y', test_window, args: ['-p', '/0039-a11y'], env: ui_a11y_env, suite: 'ui', timeout: 120, is_parallel: false)

ui_vulkan_env = environment()
ui_vulkan_env.set('GSK_RENDERER', 'vulkan')
ui_vulkan_env.set('GTK_A11Y', 'none')
ui_vulkan_env.set('EASEL_TEST_LANGUAGE', 'en')
ui_vulkan_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
ui_vulkan_env.set('GDK_DEBUG', 'no-portals')
test('ui-0025-vulkan', test_window, args: ['-p', '/0025/as1_squares_at_whole_zooms', '-p', '/0025/as1c_edges_at_fractional_zoom', '-p', '/0025/as1d_grid_on_cell_edges', '-p', '/0025/as2c_pointer_hits_the_drawn_pixel'],
     env: ui_vulkan_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0027', test_window, args: ['-p', '/0027'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0024', test_window, args: ['-p', '/0024'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
# AS-7 (0024): the desktop's own validators, and AS-8: an install finds its files.
test('desktop-entry', find_program('desktop-file-validate'), args: [desktop_file], suite: 'core')
test('metainfo', find_program('appstreamcli'), args: ['validate', '--no-net', metainfo_file], suite: 'core')
# 0024: one version in meson.build, the deb's changelog and the metainfo's newest release.
test('version-sync', find_program('python3'), args: [meson.current_source_dir() / 'version-sync.py', meson.project_source_root()], suite: 'core')
# AS-9 (0024): the Flatpak asks for exactly what REQ-0024-10 says.
test('flatpak-permissions', find_program('python3'), args: [meson.current_source_dir() / 'flatpak-permissions.py', meson.project_source_root() / 'build-aux/flatpak/io.github.jackal_kr.Easel.yml'], suite: 'core')
# AS-8 (0024): an install finds its catalogs and Help pages, with no path of the build.
test('install-run', find_program('sh'), args: [meson.current_source_dir() / 'install-run.sh', meson.project_source_root(), meson.project_build_root()], suite: 'core', is_parallel: false, timeout: 120)
# 0028: the journeys. Journey 15 in its own process, in Ukrainian under the C locale, from the settings file.
test('ui-0028', test_window, args: ['-p', '/0028'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0029', test_window, args: ['-p', '/0029'], env: ui_env, suite: 'ui', timeout: 120, is_parallel: false)
test('ui-0030', test_window, args: ['-p', '/0030'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0030-scale2', test_window, args: ['-p', '/0030-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0031', test_window, args: ['-p', '/0031'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0031-scale2', test_window, args: ['-p', '/0031-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0032', test_window, args: ['-p', '/0032'], env: ui_a11y_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0032-scale2', test_window, args: ['-p', '/0032-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0033', test_window, args: ['-p', '/0033'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0033-scale2', test_window, args: ['-p', '/0033-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0034', test_window, args: ['-p', '/0034'], env: ui_a11y_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0035', test_window, args: ['-p', '/0035'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0037', test_window, args: ['-p', '/0037'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0038', test_window, args: ['-p', '/0038'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0039', test_window, args: ['-p', '/0039'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
test('ui-0036', test_window, args: ['-p', '/0036'], env: ui_env, suite: 'ui', timeout: 300, is_parallel: false)
journey_uk_env = environment()
journey_uk_env.set('GSK_RENDERER', 'cairo')
journey_uk_env.set('GTK_A11Y', 'none')
journey_uk_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
journey_uk_env.set('GDK_DEBUG', 'no-portals')
journey_uk_env.set('LC_ALL', 'C')
journey_uk_env.set('EASEL_CHOSEN', 'uk')
test('ui-0028-uk', test_window, args: ['-p', '/0028-uk'], env: journey_uk_env, suite: 'ui', timeout: 120, is_parallel: false)
test('ui-0028-scale2', test_window, args: ['-p', '/0028-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
# AS-1b of 0025 needs a screen of scale 2 and skips elsewhere: tools/isolated.sh --scale 2 meson test -C build ui-0025-scale2
test('ui-0025-scale2', test_window, args: ['-p', '/0025-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0019-scale2', test_window, args: ['-p', '/0019-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0021-scale2', test_window, args: ['-p', '/0021-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0022-scale2', test_window, args: ['-p', '/0022-scale2'], env: ui_a11y_env, suite: 'ui', timeout: 180, is_parallel: false)
test('ui-0038-scale2', test_window, args: ['-p', '/0038-scale2'], env: ui_env, suite: 'ui', timeout: 180, is_parallel: false)
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
benchmark('perf-0034', executable('perf-0034', 'perf/perf-0034.c', dependencies: easel_core_dep), env: perf_env,
  timeout: 300)
benchmark('perf-0008', executable('perf-0008', 'perf/perf-0008.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 600)
benchmark('perf-0025', executable('perf-0025', 'perf/perf-0025.c', dependencies: easel_ui_dep), env: perf_env,
  timeout: 600)

# Languages (0023): every translation complete and right, and the bad examples refused.
check_translations = find_program('python3')
test('translations', check_translations, args: [meson.project_source_root() / 'tools/check-translations.py', '--all', meson.project_source_root() / 'po'], suite: 'core')
test('translations-refuse', check_translations, args: [meson.project_source_root() / 'tools/check-translations.py', '--refuse', meson.current_source_dir() / 'data/po'], suite: 'core')

# One run per language: a process has one language (REQ-0023-05). The Help page is found as in an uninstalled build.
foreach code : ['cs', 'de', 'en', 'es', 'fr', 'hu', 'it', 'pl', 'pt', 'ro', 'ru', 'uk']
  lang_env = environment()
  lang_env.set('GSK_RENDERER', 'cairo')
  lang_env.set('GTK_A11Y', 'none')
  lang_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
  lang_env.set('GDK_DEBUG', 'no-portals')
  lang_env.set('EASEL_TEST_LANGUAGE', code)
  test('ui-0023-' + code, test_window, args: ['-p', '/0023-lang'], env: lang_env, suite: 'ui', timeout: 180, is_parallel: false)

  # The only locale installed is C (REQ-0023-05): a user's start, with the language in the settings file.
  only_c_env = environment()
  only_c_env.set('GSK_RENDERER', 'cairo')
  only_c_env.set('GTK_A11Y', 'none')
  only_c_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
  only_c_env.set('GDK_DEBUG', 'no-portals')
  only_c_env.set('LOCPATH', '/nonexistent')
  only_c_env.set('LC_ALL', 'C')
  only_c_env.set('EASEL_ONLY_C', '1')
  only_c_env.set('EASEL_CHOSEN', code)
  only_c_env.set('EASEL_EXPECTED', code)
  test('ui-0023-only-c-' + code, test_window, args: ['-p', '/0023-start', '-p', '/0023-lang'], env: only_c_env, suite: 'ui', timeout: 180, is_parallel: false)
endforeach

# The system's language, with the setting at auto (AS-1, AS-2): each list and the language it gives.
foreach run : [['de-at', 'de_AT', 'de'], ['pt-pt', 'pt_PT', 'pt'], ['ja-pl', 'ja:pl', 'pl'], ['uk', 'uk_UA', 'uk'], ['ja', 'ja_JP', 'en']]
  auto_env = environment()
  auto_env.set('GSK_RENDERER', 'cairo')
  auto_env.set('GTK_A11Y', 'none')
  auto_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
  auto_env.set('GDK_DEBUG', 'no-portals')
  auto_env.set('LANGUAGE', run[1])
  auto_env.set('EASEL_EXPECTED', run[2])
  test('ui-0023-auto-' + run[0], test_window, args: ['-p', '/0023-start'], env: auto_env, suite: 'ui', timeout: 180, is_parallel: false)
endforeach

# AS-7: a right-to-left system.
rtl_env = ui_env
rtl_env.set('EASEL_SYSTEM_RTL', '1')
test('ui-0023-rtl', test_window, args: ['-p', '/0023-rtl'], env: rtl_env, suite: 'ui', timeout: 180, is_parallel: false)

# AS-11: the long test language, built here and never installed.
long_language = custom_target('long-language',
  output: 'long-locale.stamp',
  command: [check_translations, meson.project_source_root() / 'tools/po-long.py', meson.project_source_root() / 'po', meson.current_build_dir() / 'long-locale', '@OUTPUT@'],
  depend_files: files('../tools/po-long.py', '../tools/check-translations.py', '../po/de.po', '../po/ru.po', '../po/easel.pot'),
  build_by_default: true)
long_env = environment()
long_env.set('GSK_RENDERER', 'cairo')
long_env.set('GTK_A11Y', 'none')
long_env.set('EASEL_TEST_DATA', meson.current_source_dir() / 'data')
long_env.set('GDK_DEBUG', 'no-portals')
long_env.set('EASEL_TEST_LANGUAGE', 'xl')
long_env.set('EASEL_LOCALE_DIR', meson.current_build_dir() / 'long-locale')
test('ui-0023-long', test_window, args: ['-k', '-p', '/0023-long'], env: long_env, depends: long_language, suite: 'ui', timeout: 300, is_parallel: false)

# AS-13: a German system, and the suite still compares English.
suite_de_env = ui_env
suite_de_env.set('LANGUAGE', 'de')
test('ui-0023-suite-in-english', test_window, args: ['-p', '/0020'], env: suite_de_env, suite: 'ui', timeout: 600, is_parallel: false)

# Remove background (spec 0035, ADR-0007): one result at 8000 x 8000 against 1 s; a benchmark, never in the suite.
benchmark('perf-0035', executable('perf-0035', 'perf/perf-0035.c', dependencies: easel_core_dep), env: perf_env, timeout: 300)
benchmark('perf-0039', executable('perf-0039', 'perf/perf-0039.c', dependencies: easel_ui_dep), env: perf_env, timeout: 600)
