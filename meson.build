project('osdikern', 'c')

add_project_arguments(
  '-ffreestanding',
  language: 'c',
)

add_project_link_arguments(
  '-nostdlib',
  '-T',
  join_paths(meson.current_source_dir(), 'arch/aarch64/linker.ld'),
  language: 'c',
)

sources = [
  'arch/aarch64/init.S',
  'con/rpi3_mini_uart.c',
  'con/rpi3_mini_uart.S',
  'init/main.c',
]

elf = executable('kernel8.elf', sources, include_directories: include_directories('include'))

objcopy = find_program(meson.get_external_property('prefix') + 'objcopy')

custom_target('rpi3-image',
  output: 'kernel8.img',
  input: elf,
  command: [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'],
  build_by_default: true,
)
