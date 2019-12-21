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
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
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

int main(void) {
  // Allocate output image
  image_t image;
  image.width = 512;
  image.height = 512;
  image.pixels = malloc(image.width * image.height * sizeof(color_t));

  // Fill the background
  for (int x = 0; x < image.width; ++x) {
    for (int y = 0; y < image.height; ++y) {
      image_pixel(&image, x, y) = (color_t) {.r = 80, .g = 80, .b = 140, .a = 255};
    }
  }

  // Try to write the output image
  if (!stbi_write_png("output.png", image.width, image.height, 4, image.pixels, 0)) {
    fprintf(stderr, "error: failed to write output image\n");
    return 1;
  }

  // Clean up output image
  free(image.pixels);
}
