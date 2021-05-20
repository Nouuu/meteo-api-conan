cmake_minimum_required(VERSION 3.16)
project(meteoooo C)

set(CMAKE_C_STANDARD 11)

include(build/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(meteoooo main.c)

target_link_libraries(meteoooo ${CONAN_LIBS})