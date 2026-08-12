project('genconfig', 'c', 'cpp', version : '0.1', default_options : ['cpp_std=c++17'])

gen_config_script_path = files('gen_config.py')

cnode_dep = declare_dependency(
  include_directories : include_directories('.'),
  sources : ['cnode.h'],
)

subdir('tests')
