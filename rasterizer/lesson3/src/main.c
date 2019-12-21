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

/** 2D vector. */
typedef struct {
  float x;
  float y;
} vec2_t;

/** 3D vector. */
typedef struct {
  float x;
  float y;
  float z;
} vec3_t;

/** Fill a triangle. */
static void triangle(image_t* o_color, image_t* o_depth, vec2_t a, vec2_t b, vec2_t c, color_t color) {
  // Opposite corners of bounding box wrapping the triangle
  // These are clipped at the color buffer boundaries
  vec2_t aabb1 = {
    .x = max(0.0f, min(a.x, min(b.x, c.x))),
    .y = max(0.0f, min(a.y, min(b.y, c.y))),
  };
  vec2_t aabb2 = {
    .x = min((float) o_color->width, max(a.x, max(b.x, c.x))),
    .y = min((float) o_color->height, max(a.y, max(b.y, c.y))),
  };

  // The vector AB
  vec2_t ab = {
    .x = b.x - a.x,
    .y = b.y - a.y,
  };

  // The vector AC
  vec2_t ac = {
    .x = c.x - a.x,
    .y = c.y - a.y,
  };

  // Iterate over the bounding box
  // We will check each of its interior pixels if it belongs to the triangle
  for (int x = (int) aabb1.x; x < (int) (0.5f + aabb2.x); ++x) {
    for (int y = (int) aabb1.y; y < (int) (0.5f + aabb2.y); ++y) {
      // The vector AP
      vec2_t ap = {
        (float) x - a.x,
        (float) y - a.y,
      };

      // Find normalized barycentric coordinates tuple (u, v, w)
      float denominator = (float) ab.x * (float) ac.y - (float) ac.x * (float) ab.y;
      float v = (float) (ap.x * ac.y - ac.x * ap.y) / denominator;
      float w = (float) (ab.x * ap.y - ap.x * ab.y) / denominator;
      float u = 1.0f - v - w;

      // If all components are nonnegative, we are inside
      if (u >= 0 && v >= 0 && w >= 0) {
        image_pixel(o_color, x, y) = color;
      }
    }
  }
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

  // Draw a triangle
  triangle(&o_color, &o_depth, (vec2_t) {100, 240}, (vec2_t) {460, 300}, (vec2_t) {100, 350}, (color_t) {.r = 255, .a = 255});

  // Try to save the color buffer
  if (image_write_png(&o_color, "output.png")) {
    fprintf(stderr, "error: failed to save color buffer\n");
    return 1;
  }

  // Clean up output
  free(o_depth.pixels);
  free(o_color.pixels);
}
