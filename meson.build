project('mzm', 'c', 'cpp')

subdir('src')

executable(
    meson.project_name(),
    sources,
    include_directories: include_directories('include'),
    dependencies: [
        dependency('sdl3'),
        dependency('sdl3-ttf'),
    ],
)
