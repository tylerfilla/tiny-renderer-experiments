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
  return self->pixels[x + y * self->width];
}

// Set a pixel in a PNG image
static void png_pixel_set(png_t* self, int x, int y, color_t color) {
  self->pixels[x + y * self->width] = color;
}

// Write a PNG image
static int png_write(const png_t* self, const char* filename) {
  return !stbi_write_png(filename, self->width, self->height, 4, self->pixels, 0);
}

// Draw a simple line
static void line(png_t* image, int x0, int y0, int x1, int y1, color_t color) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  if (dx > dy) {
    for (int x = x0; x < x1; ++x) {
      float t = (float) (x - x0) / (float) dx;
      int y = y0 + (int) (t * (float) dy);
      png_pixel_set(image, x, y, color);
    }
  } else {
    for (int y = y0; y < y1; ++y) {
      float t = (float) (y - y0) / (float) dy;
      int x = x0 + (int) (t * (float) dx);
      png_pixel_set(image, x, y, color);
    }
  }
}

int main(void) {
  // Create image for output
  png_t* image;
  {
    png_params_t params;
    params.width = 256;
    params.height = 256;
    image = png_create(&params);
  }

  // Fill background with a solid color
  for (int x = 0; x < image->width; ++x) {
    for (int y = 0; y < image->height; ++y) {
      png_pixel_set(image, x, y, (color_t) {.r = 255, .g = 255, .b = 255, .a = 255});
    }
  }

  // Draw a line
  line(image, 20, 50, 123, 240, (color_t) {.a = 255});

  // Save the output image
  if (png_write(image, "output.png")) {
    fprintf(stderr, "failed to write output image\n");
    return 1;
  }

  // Clean up
  png_destroy(image);
}
