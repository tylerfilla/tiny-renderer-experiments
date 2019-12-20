#
# renderer - WTFPL
#

cmake_minimum_required(VERSION 3.4)
project(renderer)

add_executable(main src/main.c)
set_target_properties(main PROPERTIES C_STANDARD 99)
