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

  if (!stbi_write_png("output.png", image_width, image_height, 4, image_data, image_width)) {
    fprintf(stderr, "failed to write output image\n");
  }

  free(image_data);
}
