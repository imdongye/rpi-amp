cmake_minimum_required(VERSION 3.0.0)

project(amp VERSION 0.0.1)

# set(headers tsf.h tml.h examples/minisdl_audio.h)

# set(source examples/example1.c)
set(source main.c)

set(exe_name amp_exe)
if(WIN32)
    add_executable(${exe_name} ${source} examples/minisdl_audio.c)
else()
    add_executable(${exe_name} ${source})
    target_compile_options(${exe_name} PRIVATE -Wall)
    # m : 수학함수lib
    # dl : 동적링킹
    # SDL2 : minisdl_audio.c를 직접 컴파일하지 않고 동적링킹한다.
    target_link_libraries(${exe_name} PRIVATE m dl pthread SDL2)
endif()