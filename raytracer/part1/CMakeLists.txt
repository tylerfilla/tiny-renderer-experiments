#
# Renderer Experiments
# Copyright (c) 2019 Tyler Filla
#
# This work is released under the WTFPL. See the LICENSE file for details.
#

cmake_minimum_required(VERSION 3.4)
project(Raytracer1)

set(raytracer1_SRC_FILES
        src/main.c)

add_executable(raytracer1 ${raytracer1_SRC_FILES})
set_target_properties(raytracer1 PROPERTIES C_STANDARD 99)
