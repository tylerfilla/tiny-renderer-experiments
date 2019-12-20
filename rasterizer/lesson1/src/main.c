/*
 * renderer - WTFPL
 */

// Rasterizer Lesson 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// An RGBA color
typedef union color {
  struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  uint32_t value;
} color_t;

// A PNG image
typedef struct png {
  int width;
  int height;
  color_t* pixels;
} png_t;

// Construction parameters for a PNG image
typedef struct png_params {
  int width;
  int height;
} png_params_t;

// Create a PNG image instance
#define png_create(params) \
    ((png_t*) png_construct((png_t*) malloc(sizeof(png_t)), (const png_params_t*) (params)))

// Destroy a PNG image instance
#define png_destroy(self) \
    ((void) free(png_destruct((png_t*) (self))))

// Construct a PNG image instance
static png_t* png_construct(png_t* self, const png_params_t* params) {
  self->width = params->width;
  self->height = params->height;
  self->pixels = calloc(params->width * params->height, sizeof(color_t));
  return self;
}

// Destruct a PNG image instance
static png_t* png_destruct(png_t* self) {
  free(self->pixels);
  return self;
}

// Get a pixel in a PNG image
static color_t png_pixel_get(const png_t* self, int x, int y) {
  if (x < 0 || y < 0 || x >= self->width || y >= self->height) {
    return (color_t) {0};
  }
  return self->pixels[x + y * self->width];
}

// Set a pixel in a PNG image
static void png_pixel_set(png_t* self, int x, int y, color_t color) {
  if (x < 0 || y < 0 || x >= self->width || y >= self->height) {
    return;
  }
  self->pixels[x + y * self->width] = color;
}

// Write a PNG image
static int png_write(const png_t* self, const char* filename) {
  return !stbi_write_png(filename, self->width, self->height, 4, self->pixels, 0);
}

// Draw a simple line
static void line(png_t* image, int x0, int y0, int x1, int y1, color_t color) {
  // Distances on each axis
  int dx = x1 - x0;
  int dy = y1 - y0;

  // If the x-axis projection is longer
  if (abs(dx) > abs(dy)) {
    // Flip points if needed
    if (x0 > x1) {
      int x0_old = x0;
      int y0_old = y0;
      x0 = x1;
      y0 = y1;
      x1 = x0_old;
      y1 = y0_old;
      dx *= -1;
    }

    // Iterate on x-axis
    for (int x = x0; x < x1; ++x) {
      float t = (float) (x - x0) / (float) dx;
      int y = y0 + (int) (t * (float) (y1 - y0));
      png_pixel_set(image, x, y, color);
    }
  } else {
    // Flip points if needed
    if (y0 > y1) {
      int x0_old = x0;
      int y0_old = y0;
      x0 = x1;
      y0 = y1;
      x1 = x0_old;
      y1 = y0_old;
      dy *= -1;
    }

    // Iterate on y-axis
    for (int y = y0; y < y1; ++y) {
      float t = (float) (y - y0) / (float) dy;
      int x = x0 + (int) (t * (float) (x1 - x0));
      png_pixel_set(image, x, y, color);
    }
  }
}

int main(void) {
  // Create image for output
  png_t* image;
  {
    png_params_t params;
    params.width = 512;
    params.height = 512;
    image = png_create(&params);
  }

  // The background color
  color_t bg = {.r = 80, .g = 80, .b = 140, .a = 255};

  // Fill background with a solid color
  for (int x = 0; x < image->width; ++x) {
    for (int y = 0; y < image->height; ++y) {
      png_pixel_set(image, x, y, bg);
    }
  }

  // Open model file
  FILE* model = fopen("data/african_head.obj", "r");
  if (!model) {
    fprintf(stderr, "failed to open model file\n");
    return 1;
  }

  // Rummage through the model file to draw lines
  char buffer[256];
  while (fgets(buffer, 256, model)) {
    // If this line describes a face
    if (buffer[0] == 'f' && buffer[1] == ' ') {
      // Save the current file offset
      long pos = ftell(model);

      // Parse out vertex indices for this face
      int v[3], _;
      sscanf(buffer, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &v[0], &_, &_, &v[1], &_, &_, &v[2], &_, &_);

      // Coordinates for vertices in the face
      // Indexed first on vertex and second on axis
      float face[3][3];

      // For each vertex in this face
      for (int i = 0; i < 3; ++i) {
        // Seek to the beginning of the model file
        fseek(model, 0, SEEK_SET);

        // Hunt down this particular vertex
        int index = 1;
        while (fgets(buffer, 256, model)) {
          // If this line describes a vertex
          if (buffer[0] == 'v' && buffer[1] == ' ') {
            // If this is the vertex we are looking for
            if (index == v[i]) {
              // Parse out the vertex coordinates
              double x, y, z;
              sscanf(buffer, "v %lf %lf %lf\n", &x, &y, &z);

              // Store face vertex coordinates
              face[i][0] = (float) x;
              face[i][1] = (float) y;
              face[i][2] = (float) z;
            }
            index++;
          }
        }
      }

      // The line color
      color_t fg = {.r = 200, .g = 200, .b = 255, .a = 255};

      // For each vertex in this face
      for (int i = 0; i < 3; ++i) {
        // The current vertex
        float x = face[i][0];
        float y = face[i][1];
        float z = face[i][2];

        // The next vertex wrapped back
        float x_next = face[(i + 1) % 3][0];
        float y_next = face[(i + 1) % 3][1];
        float z_next = face[(i + 1) % 3][2];

        // Project 3D vertex to 2D screen space
        // This is super naive, so we just drop the Z axis
        int x_screen = (int) ((1.0f + x) * (float) image->width * 0.5f);
        int y_screen = (int) ((1.0f - y) * (float) image->height * 0.5f);
        int x_screen_next = (int) ((1.0f + x_next) * (float) image->width * 0.5f);
        int y_screen_next = (int) ((1.0f - y_next) * (float) image->height * 0.5f);

        // Compute midpoint z-distance of the line to do some color shifting as things get farther away
        // Since we are not depth sorting, some far-away things may render above some nearby things (oh well...)
        float factor = sqrtf(1.0f - ((z_next + z) / 2.0f + 1.0f) / 2.0f);
        color_t color = {
            .a = (int) ((1.0f - factor) * (float) fg.a + factor * (float) bg.a),
            .r = (int) ((1.0f - factor) * (float) fg.r + factor * (float) bg.r),
            .g = (int) ((1.0f - factor) * (float) fg.g + factor * (float) bg.g),
            .b = (int) ((1.0f - factor) * (float) fg.b + factor * (float) bg.b),
        };

        // Draw the line
        line(image, x_screen, y_screen, x_screen_next, y_screen_next, color);
      }

      // Restore the file offset
      fseek(model, pos, SEEK_SET);
    }
  }

  // Close model file
  fclose(model);

  // Save the output image
  if (png_write(image, "output.png")) {
    fprintf(stderr, "failed to write output image\n");
    return 1;
  }

  // Clean up
  png_destroy(image);
}
