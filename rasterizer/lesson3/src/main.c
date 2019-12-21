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

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_TGA
#include "stb_image.h"

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

/** Read an image from a file. */
static int image_read(image_t* image, const char* filename) {
  // Try to load data from the file
  int width;
  int height;
  int channels;
  stbi_uc* data = stbi_load(filename, &width, &height, &channels, 3);
  if (!data) {
    return -1;
  }

  // Rearrange up the image data like we will need
  // The caller is responsible for cleaning up the pixel data
  image->width = width;
  image->height = height;
  image->pixels = malloc(image->width * image->height * sizeof(color_t));
  for (int x = 0; x < image->width; ++x) {
    for (int y = 0; y < image->height; ++y) {
      // Compute pixel offset
      int pixel = (x + (image->height - y) * image->width) * 3;

      // Extract R, G, and B data
      // The alpha channel is assumed fully opaque
      image_pixel(image, x, y) = (color_t) {
        .r = data[pixel],
        .g = data[pixel + 1],
        .b = data[pixel + 2],
        .a = 255,
      };
    }
  }

  // Clean up loaded data
  stbi_image_free(data);

  return 0;
}

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
static void triangle(image_t* o_color, image_t* o_depth, vec3_t a, vec2_t at, vec3_t an, vec3_t b, vec2_t bt, vec3_t bn, vec3_t c, vec2_t ct, vec3_t cn, const image_t* texture) {
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
      // This is a solution using Cramer's rule instead of the thingamabob in the lesson
      float denominator = (float) (ab.x * ac.y - ac.x * ab.y);
      float v = (float) (ap.x * ac.y - ac.x * ap.y) / denominator;
      float w = (float) (ab.x * ap.y - ap.x * ab.y) / denominator;
      float u = 1.0f - v - w;

      // If all components are nonnegative, we are inside
      if (u >= 0 && v >= 0 && w >= 0) {
        // Compute depth of this pixel
        float depth = u * a.z + v * b.z + w * c.z;

        // If this pixel is above the pixel already drawn here, then draw it
        if (depth > image_pixel(o_depth, x, y).value) {
          // Interpolate the texture coordinates for this fragment
          vec2_t texcoord = {
            .x = u * at.x + v * bt.x + w * ct.x,
            .y = u * at.y + v * bt.y + w * ct.y,
          };

          // Interpolate the normal vector for this fragment
          vec3_t normal = {
            .x = u * an.x + v * bn.x + w * cn.x,
            .y = u * an.y + v * bn.y + w * cn.y,
            .z = u * an.z + v * bn.z + w * cn.z,
          };

          // Look up the texture color
          int tx = (int) (texcoord.x * (float) texture->width);
          int ty = (int) (texcoord.y * (float) texture->height);
          color_t color = image_pixel(texture, tx, ty);

          // Compute lighting intensity with a forward lamp
          float lighting = dot3(normal, (vec3_t) {.x = 0, .y = 0, .z = 1});

          // Light the fragment
          color.r *= lighting;
          color.g *= lighting;
          color.b *= lighting;

          // If the triangle is forward-facing
          if (lighting > 0) {
            // Write image data out
            image_pixel(o_color, x, y) = color;
            image_pixel(o_depth, x, y).value = depth;
          }
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

  // Vertex texture coordinate data
  int texcoords_capacity = 10;
  int texcoords_size = 0;
  vec2_t* texcoords = malloc(texcoords_capacity * sizeof(vec2_t));

  // Vertex normal coordinate data
  int normals_capacity = 10;
  int normals_size = 0;
  vec3_t* normals = malloc(normals_capacity * sizeof(vec3_t));

  /** A triangular face. */
  typedef struct {
    struct {
      int position;
      int texcoord;
      int normal;
    } a;
    struct {
      int position;
      int texcoord;
      int normal;
    } b;
    struct {
      int position;
      int texcoord;
      int normal;
    } c;
  } face_t;

  // Face data
  int faces_capacity = 10;
  int faces_size = 0;
  face_t* faces = malloc(faces_capacity * sizeof(face_t));

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
    } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
      // This line encodes a texture coordinate vector

      // Parse the line
      float _;
      vec2_t texcoord;
      sscanf(line, "vt %f %f %f", &texcoord.x, &texcoord.y, &_);

      // Store the parsed data
      if (texcoords_size == texcoords_capacity) {
        texcoords_capacity *= 2;
        texcoords = realloc(texcoords, texcoords_capacity * sizeof(vec3_t));
      }
      texcoords[texcoords_size] = texcoord;
      texcoords_size++;
    } else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
      // This line encodes a normal vector

      // Parse the line
      vec3_t normal;
      sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);

      // Store the parsed data
      if (normals_size == normals_capacity) {
        normals_capacity *= 2;
        normals = realloc(normals, normals_capacity * sizeof(vec3_t));
      }
      normals[normals_size] = normal;
      normals_size++;
    } else if (line[0] == 'f' && line[1] == ' ') {
      // This line encodes a face

      // Parse the line
      face_t face;
      sscanf(
          line,
          "f %d/%d/%d %d/%d/%d %d/%d/%d",
          &face.a.position,
          &face.a.texcoord,
          &face.a.normal,
          &face.b.position,
          &face.b.texcoord,
          &face.b.normal,
          &face.c.position,
          &face.c.texcoord,
          &face.c.normal);

      // Wavefront OBJ files index from one :(
      face.a.position--;
      face.a.texcoord--;
      face.a.normal--;
      face.b.position--;
      face.b.texcoord--;
      face.b.normal--;
      face.c.position--;
      face.c.texcoord--;
      face.c.normal--;

      // Store the parsed data
      if (faces_size == faces_capacity) {
        faces_capacity *= 2;
        faces = realloc(faces, faces_capacity * sizeof(face_t));
      }
      faces[faces_size] = face;
      faces_size++;
    }
  }

  // Close model file
  fclose(file);

  // Load the head texture
  image_t texture;
  if (image_read(&texture, "data/african_head_diffuse.tga")) {
    fprintf(stderr, "error: failed to read texture file\n");
    exit(1);
  }

  // Iterate over triangular faces in model
  for (int i = 0; i < faces_size; ++i) {
    face_t face = faces[i];

    // Look up vertex positions
    vec3_t p1 = positions[face.a.position];
    vec3_t p2 = positions[face.b.position];
    vec3_t p3 = positions[face.c.position];

    // Look up vertex texture coordinates
    vec2_t tc1 = texcoords[face.a.texcoord];
    vec2_t tc2 = texcoords[face.b.texcoord];
    vec2_t tc3 = texcoords[face.c.texcoord];

    // Look up vertex normal vectors
    vec3_t n1 = normals[face.a.normal];
    vec3_t n2 = normals[face.b.normal];
    vec3_t n3 = normals[face.c.normal];

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

     // Draw the transformed triangle to the output image
     // The great thing about triangles is that they stay triangles even after a mathematical shakedown
     triangle(o_color, o_depth, p1_screen, tc1, n1, p2_screen, tc2, n2, p3_screen, tc3, n3, &texture);
  }

  // Clean up head texture
  free(texture.pixels);

  // Clean up model data
  free(faces);
  free(texcoords);
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
  if (image_write_png(&o_color, "output3.png")) {
    fprintf(stderr, "error: failed to save color buffer\n");
    return 1;
  }

  // Clean up output
  free(o_depth.pixels);
  free(o_color.pixels);
}
