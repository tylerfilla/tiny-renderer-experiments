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

/** Fill a triangle. */
static void triangle(image_t* image, vec2i_t a, vec2i_t b, vec2i_t c, color_t color) {
  // Sort the points A, B, and C by y-coordinates increasing
  // This lets us make some nice assumptions later on when filling the triangle line-by-line
  if (a.y > b.y) {
    vec2i_t tmp = a;
    a = b;
    b = tmp;
  }
  if (b.y > c.y) {
    vec2i_t tmp = b;
    b = c;
    c = tmp;
  }
  if (a.y > b.y) {
    vec2i_t tmp = a;
    a = b;
    b = tmp;
  }

  // Iterate over the height of the triangle
  // Since we did the sort above, we just need to scan from point A to point C on the y-axis
  for (int y = a.y; y < c.y; ++y) {
    // The x-span at this y-coordinate
    int x0;
    int x1;

    // If we are below point B
    if (y < b.y) {
      // We are in the lower half of the triangle
      // We need to solve the AB and CA lines and fill between them

      // Calculate parametric "progress" along the AB and CA lines
      float ab_t = (float) (y - a.y) / (float) (b.y - a.y);
      float ca_t = (float) (y - a.y) / (float) (c.y - a.y); // meow

      // Calculate the x-values we must have on the AB and CA lines
      x0 = (int) ((float) a.x + ab_t * (float) (b.x - a.x));
      x1 = (int) ((float) a.x + ca_t * (float) (c.x - a.x));
    } else {
      // We are in the upper half of the triangle
      // We need to solve the BC and CA lines and fill between them

      // Calculate parametric "progress" along the BC and CA lines
      float bc_t = (float) (y - b.y) / (float) (c.y - b.y);
      float ca_t = (float) (y - a.y) / (float) (c.y - a.y); // meow

      // Calculate the x-value we must have on the BC and CA lines
      x0 = (int) ((float) b.x + bc_t * (float) (c.x - b.x));
      x1 = (int) ((float) a.x + ca_t * (float) (c.x - a.x));
    }

    // Draw the x-span we computed
    // This has the effect of filling this one row of the triangle
    line(image, (vec2i_t) {x0, y}, (vec2i_t) {x1, y}, color);
  }
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
  if (!stbi_write_png("output2.png", image.width, image.height, 4, image.pixels, 0)) {
    fprintf(stderr, "error: failed to write output image\n");
    return 1;
  }

  // Clean up image pixels
  free(image.pixels);
}
