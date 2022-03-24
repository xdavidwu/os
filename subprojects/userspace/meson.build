project('osdiuser', 'c')

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

svcelf = executable('svc.elf', 'arch/aarch64/svc.S')

objcopy = find_program(meson.get_external_property('prefix') + 'objcopy')

custom_target('svc-image',
  output: 'svc.img',
  input: svcelf,
  command: [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'],
  build_by_default: true,
)
