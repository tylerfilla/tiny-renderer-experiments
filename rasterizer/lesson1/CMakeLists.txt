#
# renderer - WTFPL
#

cmake_minimum_required(VERSION 3.4)
project(Rasterizer1)

set(rasterizer1_SRC_FILES
        src/main.c)

add_executable(rasterizer1 ${rasterizer1_SRC_FILES})
set_target_properties(rasterizer1 PROPERTIES C_STANDARD 99)
