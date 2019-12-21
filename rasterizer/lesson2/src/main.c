/*
 * Renderer Experiments
 * Copyright (c) 2019 Tyler Filla
 *
 * This work is released under the WTFPL. See the LICENSE file for details.
 */

//
// Rasterizer - Lesson 2
//

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

  // Vertex position data
  size_t positions_capacity = 10;
  size_t positions_size = 0;
  vec3f_t* positions = malloc(positions_capacity * sizeof(vec3f_t));

  // Face data
  size_t faces_capacity = 10;
  size_t faces_size = 0;
  vec3i_t* faces = malloc(faces_capacity * sizeof(vec3i_t));

  // Open up our model file for read
  FILE* file = fopen("data/african_head.obj", "r");

  // Crudely parse model data from the file into memory
  // I am making so many assumptions here it's not even funny
  char line[256];
  while (fgets(line, 256, file)) {
    // Ignore comment lines
    if (line[0] == '#') {
      continue;
    }

    // Deal with meaningful lines
    if (line[0] == 'v' && line[1] == ' ') {
      // This line encodes a position vector

      // Parse the line
      vec3f_t position;
      sscanf(line, "v %f %f %f", &position.x, &position.y, &position.z);

      // Store the parsed data
      if (positions_size == positions_capacity) {
        positions_capacity *= 2;
        positions = realloc(positions, positions_capacity * sizeof(vec3f_t));
      }
      positions[positions_size] = position;
      positions_size++;
    } else if (line[0] == 'f' && line[1] == ' ') {
      // This line encodes a face

      // Parse the line
      int _;
      vec3i_t face;
      sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &face.x, &_, &_, &face.y, &_, &_, &face.z, &_, &_);

      // Store the parsed data
      if (faces_size == faces_capacity) {
        faces_capacity *= 2;
        faces = realloc(faces, faces_capacity * sizeof(vec3f_t));
      }
      faces[faces_size] = face;
      faces_size++;
    }
  }

  // Close model file
  fclose(file);

  // Iterate over triangular faces in model
  for (int i = 0; i < faces_size; ++i) {
    vec3i_t face = faces[i];

    // Look up vertex positions in 3D space
    // Keep in mind that OBJ files are one-indexed :(
    vec3f_t p1 = positions[face.x - 1];
    vec3f_t p2 = positions[face.y - 1];
    vec3f_t p3 = positions[face.z - 1];

    // Project these vertices into our 2D screen space
    // This is naive just like in lesson 1 (we just drop the Z-axis altogether!)
    vec2f_t p1_screen = {
      .x = (1.0f + p1.x) * (float) image.width * 0.5f,
      .y = (1.0f - p1.y) * (float) image.height * 0.5f,
    };
    vec2f_t p2_screen = {
      .x = (1.0f + p2.x) * (float) image.width * 0.5f,
      .y = (1.0f - p2.y) * (float) image.height * 0.5f,
    };
    vec2f_t p3_screen = {
      .x = (1.0f + p3.x) * (float) image.width * 0.5f,
      .y = (1.0f - p3.y) * (float) image.height * 0.5f,
    };

    // Draw the transformed triangle to the output image
    // The great thing about triangles is that they stay triangles even after a mathematical shakedown
    triangle(&image, vec2f2i(p1_screen), vec2f2i(p2_screen), vec2f2i(p3_screen), (color_t) {
      .r = rand() % 255,
      .g = rand() % 255,
      .b = rand() % 255,
      .a = 255,
    });
  }

  // Clean up model data
  free(faces);
  free(positions);

  // Try to write the output image
  if (!stbi_write_png("output4.png", image.width, image.height, 4, image.pixels, 0)) {
    fprintf(stderr, "error: failed to write output image\n");
    return 1;
  }

  // Clean up image pixels
  free(image.pixels);
}
