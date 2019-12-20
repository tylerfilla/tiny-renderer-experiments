/*
 * Renderer Experiments
 * Copyright (c) 2019 Tyler Filla
 *
 * This work is released under the WTFPL. See the LICENSE file for details.
 */

//
// Rasterizer - Lesson 2
//

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/** An RGBA color. */
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
    (((image_t*) (image))->pixels[x + y * ((image_t*) (image))->width])

/** An integer 2-vector. */
typedef struct {
  int x;
  int y;
} vec2i_t;

/** Draw an aliased line. */
static void line(image_t* image, vec2i_t a, vec2i_t b, color_t color) {
  // Displacements
  int dx = b.x - a.x;
  int dy = b.y - a.y;

  // If the line is wider than it is tall
  // We need to iterate on the longer axis to prevent stippling
  if (abs(dx) > abs(dy)) {
    // Swap points if they are reversed on the x-axis
    if (a.x > b.x) {
      vec2i_t c = a;
      a = b;
      b = c;
      dx = -dx;
      dy = -dy;
    }

    // Iterate over the x-axis, and draw the line
    for (int x = a.x; x < b.x; ++x) {
      float t = (float) (x - a.x) / (float) dx;
      int y = (int) ((float) a.y + t * (float) dy);
      image_pixel(image, x, y) = color;
    }
  } else {
    // Swap points if they are reversed on the y-axis
    if (a.y > b.y) {
      vec2i_t c = a;
      a = b;
      b = c;
      dx = -dx;
      dy = -dy;
    }

    // Iterate over the y-axis, and draw the line
    for (int y = a.y; y < b.y; ++y) {
      float t = (float) (y - a.y) / (float) dy;
      int x = (int) ((float) a.x + t * (float) dx);
      image_pixel(image, x, y) = color;
    }
  }
}

/** Draw an aliased triangle. */
static void triangle(image_t* image, vec2i_t a, vec2i_t b, vec2i_t c, color_t color) {
  line(image, a, b, color);
  line(image, b, c, color);
  line(image, c, a, color);
}

int main(void) {
  // Allocate output image
  image_t image;
  image.width = 512;
  image.height = 512;
  image.pixels = calloc(image.width * image.height, sizeof(color_t));

  // Add background fill
  for (int x = 0; x < image.width; ++x) {
    for (int y = 0; y < image.height; ++y) {
      image_pixel(&image, x, y) = (color_t) {.r = 80, .g = 80, .b = 140, .a = 255};
    }
  }

  // Draw some triangles
  triangle(&image, (vec2i_t) {100, 100}, (vec2i_t) {140, 50}, (vec2i_t) {200, 400}, (color_t) {.r = 255, .a = 255});
  triangle(&image, (vec2i_t) {100, 401}, (vec2i_t) {432, 213}, (vec2i_t) {503, 357}, (color_t) {.g = 255, .a = 255});
  triangle(&image, (vec2i_t) {314, 100}, (vec2i_t) {377, 378}, (vec2i_t) {231, 503}, (color_t) {.b = 255, .a = 255});

  // Try to write the output image
  if (!stbi_write_png("output.png", image.width, image.height, 4, image.pixels, 0)) {
    fprintf(stderr, "error: failed to write output image\n");
    return 1;
  }

  // Clean up image pixels
  free(image.pixels);
}
