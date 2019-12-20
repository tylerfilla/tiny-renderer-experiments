/*
 * renderer - WTFPL
 */

// Rasterizer Lesson 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct point {
  float x;
  float y;
} point_t;

static void line(uint32_t* image, int width, const point_t* p0, const point_t* p1, uint32_t color) {
  for (float t = 0; t < 1.0f; t += 0.01f) {
    int x = (int) (p0->x + (p1->x - p0->x) * t);
    int y = (int) (p0->y + (p1->y - p0->y) * t);
    image[x + y * width] = color;
  }
}

int main(void) {
  srand((unsigned int) time(NULL));

  const int image_width = 256;
  const int image_height = 256;
  uint32_t* image_data = calloc(image_width * image_height, sizeof(uint32_t));

  for (int x = 0; x < image_width; ++x) {
    for (int y = 0; y < image_height; ++y) {
      image_data[x + y * image_width] = (0xff << 24) // alpha
          | ((rand() % 0x7f + 0x7e) << 16) // blue
          | ((rand() % 0x7f + 0x7e) << 8) // green
          | ((rand() % 0x7f + 0x7e) << 0); // red
    }
  }

  line(image_data, image_width, &(point_t) {20,40}, &(point_t) {50,100}, 0xff000000);

  if (!stbi_write_png("output.png", image_width, image_height, 4, image_data, 0)) {
    fprintf(stderr, "failed to write output image\n");
  }

  free(image_data);
}
