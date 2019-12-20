#
# Renderer Experiments
# Copyright (c) 2019 Tyler Filla
#
# This work is released under the WTFPL. See the LICENSE file for details.
#

cmake_minimum_required(VERSION 3.4)
project(Raytracer2)

set(raytracer2_SRC_FILES
        src/main.c)

add_executable(raytracer2 ${raytracer2_SRC_FILES})
set_target_properties(raytracer2 PROPERTIES C_STANDARD 99)
