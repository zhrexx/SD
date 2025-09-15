cmake_minimum_required(VERSION 4.0)
project(SD C)

set(CMAKE_C_STANDARD 23)

add_executable(SD main.c)
target_compile_options(SD PRIVATE -municode)
target_link_libraries(SD PRIVATE winmm)
target_link_libraries(SD PRIVATE gdi32)

