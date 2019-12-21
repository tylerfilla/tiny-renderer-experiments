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

/** 2-vector with integer components. */
typedef struct {
  int x;
  int y;
} vec2i_t;

/** 2-vector with floating point components. */
typedef struct {
  float x;
  float y;
} vec2f_t;

/** Convert vec2i_t to vec2f_t. */
inline static vec2f_t vec2i2f(vec2i_t v) {
  return (vec2f_t) {
    .x = (float) v.x,
    .y = (float) v.y,
  };
}

/** Convert vec2f_t to vec2i_t. */
inline static vec2i_t vec2f2i(vec2f_t v) {
  return (vec2i_t) {
    .x = (int) v.x,
    .y = (int) v.y,
  };
}

/** 3-vector with integer components. */
typedef struct {
  int x;
  int y;
  int z;
} vec3i_t;

/** 3-vector with floating point components. */
typedef struct {
  float x;
  float y;
  float z;
} vec3f_t;

/** Convert vec3i_t to vec3f_t. */
inline static vec3f_t vec3i2f(vec3i_t v) {
  return (vec3f_t) {
    .x = (float) v.x,
    .y = (float) v.y,
    .z = (float) v.z,
  };
}

/** Convert vec3f_t to vec3i_t. */
inline static vec3i_t vec3f2i(vec3f_t v) {
  return (vec3i_t) {
    .x = (int) v.x,
    .y = (int) v.y,
    .z = (int) v.z,
  };
}

/** Truncate vec3i_t to vec2i_t. */
inline static vec2i_t trunc3i2(vec3i_t v) {
  return (vec2i_t) {
      .x = v.x,
      .y = v.y,
  };
}

/** Pad vec2i_t to vec3i_t. */
inline static vec3i_t pad2i3(vec2i_t v, int z) {
  return (vec3i_t) {
      .x = v.x,
      .y = v.y,
      .z = z,
  };
}

/** Truncate vec3f_t to vec2f_t. */
inline static vec2f_t trunc3f2(vec3f_t v) {
  return (vec2f_t) {
    .x = v.x,
    .y = v.y,
  };
}

/** Pad vec2f_t to vec3f_t. */
inline static vec3f_t pad2f3(vec2f_t v, float z) {
  return (vec3f_t) {
      .x = v.x,
      .y = v.y,
      .z = z,
  };
}

/** Cross product between two 3-vectors with floating point components. */
inline static vec3f_t cross3f(vec3f_t a, vec3f_t b) {
  return (vec3f_t) {
    .x = a.y * b.z - a.z * b.y,
    .y = a.z * b.x - a.x * b.z,
    .z = a.x * b.y - a.y * b.x,
  };
}

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
  // Points that define the on-screen part of the bounding box
  // This is the tightest visible axis-aligned bounding box containing points A, B, and C
  vec2i_t bbox1 = {
    .x = max(0, min(a.x, min(b.x, c.x))),
    .y = max(0, min(a.y, min(b.y, c.y))),
  };
  vec2i_t bbox2 = {
    .x = min(image->width, max(a.x, max(b.x, c.x))),
    .y = min(image->height, max(a.y, max(b.y, c.y))),
  };

  // The tutorial explanation felt a little wonky IMHO, so I solve the system using Cramer's rule here

  // Vector from A to B
  vec2i_t ab = {
    .x = b.x - a.x,
    .y = b.y - a.y,
  };

  // Vector from A to C
  vec2i_t ac = {
    .x = c.x - a.x,
    .y = c.y - a.y,
  };

  // Iterate over all pixels in the bounding box
  for (int x = bbox1.x; x < bbox2.x; ++x) {
    for (int y = bbox1.y; y < bbox2.y; ++y) {
      // Vector from A to (x, y)
      vec2i_t ap = {
        .x = x - a.x,
        .y = y - a.y,
      };

      // Common denominator (see Cramer's rule)
      float den = (float) ab.x * (float) ac.y - (float) ac.x * (float) ab.y;

      // Normalized barycentric coordinates
      float v = (float) (ap.x * ac.y - ac.x * ap.y) / den;
      float w = (float) (ab.x * ap.y - ap.x * ab.y) / den;
      float u = 1.0f - v - w;

      // If all components are nonnegative, we are in the triangle :)
      if (u >= 0 && v >= 0 && w >= 0) {
        image_pixel(image, x, y) = color;
      }
    }
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
  if (!stbi_write_png("output3.png", image.width, image.height, 4, image.pixels, 0)) {
    fprintf(stderr, "error: failed to write output image\n");
    return 1;
  }

  // Clean up image pixels
  free(image.pixels);
}
