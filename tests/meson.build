py3 = import('python').find_installation('python3')

tomlplusplus_dep = dependency('tomlplusplus')

test(
  'gen_config_python_test',
  py3,
  args : ['-m', 'unittest', 'discover', '-s', meson.current_source_dir()],
  workdir : meson.project_source_root()
)

test_cnode_exe = executable(
  'test_cnode',
  'test_cnode.cpp',
  dependencies : [cnode_dep, tomlplusplus_dep]
)
test('cnode_cpp_test', test_cnode_exe)

test_config_h = custom_target(
  'test_config.h',
  input : 'test_spec.yml',
  output : 'test_config.h',
  command : [py3, gen_config_script_path, '@INPUT@', '@OUTPUT@']
)

test_e2e_exe = executable(
  'test_e2e',
  ['test_e2e.cpp', test_config_h],
  dependencies : [cnode_dep, tomlplusplus_dep]
)
test('e2e_cpp_test', test_e2e_exe)
