project('detonate-libretro', 'c', 'cpp',default_options: [ 'buildtype=debug', 'c_std=c11','cpp_std=c++20'])

src = []


src += [
    'src/libretro.cpp',
    'src/imgui_libretro.cpp',
    'src/glsym/rglgen.c',
    'src/glsym/glsym_gl.c'
]


srcdirs = [
    'src/deps/imgui',
    'src/mudutils',
    'src/player',
    'src/player/formats',
]

foreach srcitem : srcdirs
  glob = run_command('python3', 'src_batch.py',srcitem)
  src += glob.stdout().strip().split('\n')
endforeach

inc = [
  include_directories(
    'src/deps/imgui',
    'src/deps',
    'src/deps/decoder',
    'src/deps/decoder/m4p',
    'src/deps/decoder/m4p',
    'src/deps/decoder/musepack',
    'src/deps/decoder/opus',
    'src/deps/decoder/opus/celt',
    'src/deps/decoder/opus/libopus',
    'src/deps/decoder/opus/opusfile',
    'src/deps/decoder/opus/silk',
    'src/deps/decoder/wavpack',
    'src',
    'src/mudutils',
    'src/player/formats',
    'src/player',
    'src/deps/stb',
    'src/glsym'
  )
]

if host_machine.system() == 'linux'
    dl_dep = meson.get_compiler('cpp').find_library('dl', required: true)
else
    dl_dep = dependency('', required: false)
endif

sdl2dep = dependency('gl')
fdkaac_dep = dependency('fdk-aac', required : true,static:true)

test_ldflags = []
common_ldflags = meson.get_compiler('cpp').get_supported_link_arguments (
  test_ldflags)

if host_machine.system() == 'windows'
  common_ldflags += [
    '-fno-stack-protector',
    '-lws2_32',
    '-static'
    ]
endif

add_project_link_arguments (
common_ldflags,
language: [ 'c', 'cpp' ],)
dir_base = meson.current_source_dir()
dir_install = join_paths(dir_base, 'compile_dir')
shared_library('detonate-libretro',install: true,install_dir: dir_install,include_directories : [inc],sources : [src],dependencies : [fdkaac_dep,dl_dep,sdl2dep], cpp_args: '-DLIBRETRO'
)

