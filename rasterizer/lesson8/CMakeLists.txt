#
# Renderer Experiments
# Copyright (c) 2019 Tyler Filla
#
# This work is released under the WTFPL. See the LICENSE file for details.
#

cmake_minimum_required(VERSION 3.4)
project(Rasterizer8)

set(rasterizer8_SRC_FILES
        src/main.c)

add_executable(rasterizer8 ${rasterizer8_SRC_FILES})
set_target_properties(rasterizer8 PROPERTIES C_STANDARD 99)
