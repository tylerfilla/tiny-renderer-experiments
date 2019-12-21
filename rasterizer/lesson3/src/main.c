/*
 * Renderer Experiments
 * Copyright (c) 2019 Tyler Filla
 *
 * This work is released under the WTFPL. See the LICENSE file for details.
 */

//
// Rasterizer - Lesson 3
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/** An RGBA8 color. */
typedef union {
  struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  uint32_t value;
} color_t;

/** A simple image. */
typedef struct {
  int width;
  int height;
  color_t* pixels;
} image_t;

/** Access a pixel in an image. */
#define image_pixel(image, x, y) \
    ((image_t*) (image))->pixels[(int) x + (int) (y) * ((image_t*) image)->width]

/** Write an image to a PNG file. */
static int image_write_png(const image_t* image, const char* filename) {
  return !stbi_write_png(filename, image->width, image->height, 4, image->pixels, 0);
}

int main(void) {
  // Allocate color buffer
  image_t o_color;
  o_color.width = 512;
  o_color.height = 512;
  o_color.pixels = malloc(o_color.width * o_color.height * sizeof(color_t));

  // Allocate depth buffer
  image_t o_depth;
  o_depth.width = o_color.width;
  o_depth.height = o_color.height;
  o_depth.pixels = malloc(o_depth.width * o_depth.height * sizeof(color_t));

  // Clear color buffer
  for (int x = 0; x < o_color.width; ++x) {
    for (int y = 0; y < o_color.height; ++y) {
      image_pixel(&o_color, x, y) = (color_t) {
        .r = 80,
        .g = 80,
        .b = 140,
        .a = 255
      };
    }
  }

  // Clear depth buffer
  for (int x = 0; x < o_depth.width; ++x) {
    for (int y = 0; y < o_depth.height; ++y) {
      image_pixel(&o_depth, x, y).value = INT32_MIN;
    }
  }

  // Try to save the color buffer
  if (image_write_png(&o_color, "output.png")) {
    fprintf(stderr, "error: failed to save color buffer\n");
    return 1;
  }

  // Clean up output
  free(o_depth.pixels);
  free(o_color.pixels);
}
