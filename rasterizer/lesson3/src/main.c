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
  int32_t value;
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

/** Dot product between two 3-vectors. */
inline static float dot3(vec3_t a, vec3_t b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

/** Cross product between two 3-vectors. */
inline static vec3_t cross3(vec3_t a, vec3_t b) {
  return (vec3_t) {
    .x = a.y * b.z - a.z * b.y,
    .y = a.z * b.x - a.x * b.z,
    .z = a.x * b.y - a.y * b.x,
  };
}

/** Normalize a 3-vector. */
inline static vec3_t norm3(vec3_t v) {
  float mag = sqrtf(dot3(v, v));
  return (vec3_t) {
    .x = v.x / mag,
    .y = v.y / mag,
    .z = v.z / mag,
  };
}

/** Fill a triangle. */
static void triangle(image_t* o_color, image_t* o_depth, vec3_t a, vec3_t b, vec3_t c, color_t color) {
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
  for (int x = (int) aabb1.x; x <= (int) (0.5f + aabb2.x); ++x) {
    for (int y = (int) aabb1.y; y <= (int) (0.5f + aabb2.y); ++y) {
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
        // Compute depth of this pixel
        float depth = u * a.z + v * b.z + w * c.z;

        // If this pixel is above the pixel already drawn here, then draw it
        if (depth > image_pixel(o_depth, x, y).value) {
          image_pixel(o_color, x, y) = color;
          image_pixel(o_depth, x, y).value = depth;
        }
      }
    }
  }
}

/** Draw the head model. */
static void draw(image_t* o_color, image_t* o_depth) {
  // Vertex position data
  int positions_capacity = 10;
  int positions_size = 0;
  vec3_t* positions = malloc(positions_capacity * sizeof(vec3_t));

  // Face data
  int faces_capacity = 10;
  int faces_size = 0;
  vec3_t* faces = malloc(faces_capacity * sizeof(vec3_t));

  // Open up our model file for read
  FILE* file = fopen("data/african_head.obj", "r");
  if (!file) {
    fprintf(stderr, "error: failed to open model file\n");
    exit(1);
  }

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
      vec3_t position;
      sscanf(line, "v %f %f %f", &position.x, &position.y, &position.z);

      // Store the parsed data
      if (positions_size == positions_capacity) {
        positions_capacity *= 2;
        positions = realloc(positions, positions_capacity * sizeof(vec3_t));
      }
      positions[positions_size] = position;
      positions_size++;
    } else if (line[0] == 'f' && line[1] == ' ') {
      // This line encodes a face

      // Parse the line
      int _;
      vec3_t face;
      sscanf(line, "f %f/%f/%f %f/%f/%f %f/%f/%f", &face.x, &_, &_, &face.y, &_, &_, &face.z, &_, &_);

      // Store the parsed data
      if (faces_size == faces_capacity) {
        faces_capacity *= 2;
        faces = realloc(faces, faces_capacity * sizeof(vec3_t));
      }
      faces[faces_size] = face;
      faces_size++;
    }
  }

  // Close model file
  fclose(file);

  // Iterate over triangular faces in model
  for (int i = 0; i < faces_size; ++i) {
    vec3_t face = faces[i];

    // Look up vertex positions in 3D space
    // Keep in mind that OBJ files are one-indexed :(
    vec3_t p1 = positions[(int) face.x - 1];
    vec3_t p2 = positions[(int) face.y - 1];
    vec3_t p3 = positions[(int) face.z - 1];

    // The vector from P1 to P2
    vec3_t p1p2 = {
      .x = p2.x - p1.x,
      .y = p2.y - p1.y,
      .z = p2.z - p1.z,
    };

    // The vector from P1 to P3
    vec3_t p1p3 = {
      .x = p3.x - p1.x,
      .y = p3.y - p1.y,
      .z = p3.z - p1.z,
    };

    // The normalized normal vector to the face
    vec3_t normal = norm3(cross3(p1p2, p1p3));

    // Project these vertices into our screen space
    // This is naive just like in lesson 1 (we just drop the Z-axis altogether!)
    vec3_t p1_screen = {
      .x = (1.0f + p1.x) * (float) o_color->width * 0.5f,
      .y = (1.0f - p1.y) * (float) o_color->height * 0.5f,
      .z = (1.0f + p1.z) * (float) INT32_MAX * 0.5f,
    };
    vec3_t p2_screen = {
      .x = (1.0f + p2.x) * (float) o_color->width * 0.5f,
      .y = (1.0f - p2.y) * (float) o_color->height * 0.5f,
      .z = (1.0f + p2.z) * (float) INT32_MAX * 0.5f,
    };
    vec3_t p3_screen = {
      .x = (1.0f + p3.x) * (float) o_color->width * 0.5f,
      .y = (1.0f - p3.y) * (float) o_color->height * 0.5f,
      .z = (1.0f + p3.z) * (float) INT32_MAX * 0.5f,
    };

    // Compute lighting intensity with a forward lamp
    float lighting = dot3(normal, (vec3_t) {.x = 0, .y = 0, .z = 1});

    // If the triangle is forward-facing
    if (lighting > 0) {
      // Draw the transformed triangle to the output image
      // The great thing about triangles is that they stay triangles even after a mathematical shakedown
      triangle(o_color, o_depth, p1_screen, p2_screen, p3_screen, (color_t) {
        .r = lighting * 255,
        .g = lighting * 255,
        .b = lighting * 255,
        .a = 255,
      });
    }
  }

  // Clean up model data
  free(faces);
  free(positions);
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

  // Draw the model
  draw(&o_color, &o_depth);

  // Try to save the color buffer
  if (image_write_png(&o_color, "output.png")) {
    fprintf(stderr, "error: failed to save color buffer\n");
    return 1;
  }

  // Clean up output
  free(o_depth.pixels);
  free(o_color.pixels);
}
