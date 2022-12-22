cmake_minimum_required(VERSION 3.15)

project(pipe)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -pedantic)

add_executable(pipe pipe.c)

enable_testing()
add_subdirectory(tests)
