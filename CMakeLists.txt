cmake_minimum_required(VERSION 3.28)
project(tree_but_cooler C)

set(CMAKE_C_STANDARD 23)

add_executable(tree_but_cooler main.c)

target_compile_options(tree_but_cooler PRIVATE
        -Wall
        -Wextra
        -pedantic
)