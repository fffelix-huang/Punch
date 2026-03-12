project('punch', 'cpp',
        default_options : ['cpp_std=c++20', 'b_ndebug=if-release', 'warning_level=3', 'b_lto=true', 'b_vscrt=mt'],
        meson_version: '>=0.57')

cc = meson.get_compiler('cpp')

if cc.get_id() == 'clang'
  # Thread safety annotation
  add_project_arguments('-Wthread-safety', language : 'cpp')
endif

deps = []
files = []
includes = []

includes += include_directories('src')

files += [
  'src/chess/attacks.cc',
  'src/chess/board.cc',
  'src/chess/movegen.cc',
  'src/chess/perft.cc',
  'src/engine.cc',
  'src/evaluate.cc',
  'src/movepicker.cc',
  'src/search.cc',
  'src/transposition_table.cc',
  'src/uci.cc'
]

punch_lib = library('punch_lib', files, include_directories: includes, dependencies: deps)

if get_option('gtest')
  gtest_dep = dependency('gtest')
  gtest_main_dep = dependency('gtest_main')

  test('ChessAttacksTest',
    executable('chess_attacks_test', 'src/chess/attacks_test.cc',
    include_directories: includes, link_with: punch_lib, dependencies: [gtest_dep, gtest_main_dep]
  ), args: '--gtest_output=xml:chess_attacks_test.xml', timeout: 90)
  test('ChessBoardTest',
    executable('chess_board_test', 'src/chess/board_test.cc',
    include_directories: includes, link_with: punch_lib, dependencies: [gtest_dep, gtest_main_dep]
  ), args: '--gtest_output=xml:chess_board_test.xml', timeout: 90)
  test('ChessMoveGenTest',
    executable('chess_movegen_test', 'src/chess/movegen_test.cc',
    include_directories: includes, link_with: punch_lib, dependencies: [gtest_dep, gtest_main_dep]
  ), args: '--gtest_output=xml:chess_movegen_test.xml', timeout: 90)
  test('ChessPerftTest',
    executable('chess_perft_test', 'src/chess/perft_test.cc',
    include_directories: includes, link_with: punch_lib, dependencies: [gtest_dep, gtest_main_dep]
  ), args: '--gtest_output=xml:chess_perft_test.xml', timeout: 90)
  test('ChessZobristTest',
    executable('chess_zobrist_test', 'src/chess/zobrist_test.cc',
    include_directories: includes, link_with: punch_lib, dependencies: [gtest_dep, gtest_main_dep]
  ), args: '--gtest_output=xml:chess_zobrist_test.xml', timeout: 90)
endif

if get_option('gbench')
  gbench_dep = dependency('benchmark')
  gbench_main_dep = dependency('benchmark_main')
endif

if get_option('punch')
  executable('punch', 'src/main.cc',
    files, include_directories: includes, dependencies: deps,
    install : false
  )
endif
