cmake_minimum_required(VERSION 3.0.0)

project(amp VERSION 0.0.1)

# set(headers tsf.h tml.h examples/minisdl_audio.h)

set(sources examples/minisdl_audio.c examples/example3.c)

set(exe_name amp_exe)
add_executable(${exe_name} ${sources})
target_compile_options(${exe_name} PRIVATE -Wall)
# m : 수학함수lib
# dl : 동적링킹
target_link_libraries(${exe_name} PRIVATE m dl pthread)