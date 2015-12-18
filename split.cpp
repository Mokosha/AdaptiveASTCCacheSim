#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  int x, y, n;
  unsigned char *data = stbi_load(argv[1], &x, &y, &n, 0);
  if (nullptr == data) {
    std::cerr << "Error loading file: " << argv[1] << std::endl;
    return 1;
  }

  if (x % 12 != 0 || y % 12 != 0) {
    std::cerr << "Image dimension not multiple of 12!";
    stbi_image_free(data);
    return 1;
  }

  // Compute block sizes...
  const int blocks_x = x / 12;
  const int blocks_y = y / 12;

  const int dst_x = blocks_x * 8;
  const int dst_y = blocks_y * 8;
  const int dst_sz = dst_y * dst_x;

  // Define our four split images...
  unsigned char *top_left = new unsigned char[dst_sz * n];
  unsigned char *top_right = new unsigned char[dst_sz * n];
  unsigned char *bottom_left = new unsigned char[dst_sz * n];
  unsigned char *bottom_right = new unsigned char[dst_sz * n];

  // Do the image split...
  for (int j = 0; j < y; j += 12) {
    for (int i = 0; i < x; i += 12) {
#define POPULATE_AT(dst, offset_x, offset_y) \
      do { \
      for (int q = 0; q < 8; ++q) { \
        for (int p = 0; p < 8; ++p) { \
          const int src_xx = i + p + offset_x; \
          const int src_yy = j + q + offset_y; \
          const int src_idx = (src_yy * x + src_xx) * n; \
          \
          const int dst_xx = (i / 12) * 8 + p; \
          const int dst_yy = (j / 12) * 8 + q; \
          const int dst_idx = (dst_yy * dst_x + dst_xx) * n; \
          \
          memcpy(dst + dst_idx, data + src_idx, n);  \
        } \
      } \
    } while(0)

      POPULATE_AT(top_left, 0, 0);
      POPULATE_AT(top_right, 4, 0);
      POPULATE_AT(bottom_left, 0, 4);
      POPULATE_AT(bottom_right, 4, 4);
    }
  }

  // Spit out the images
  if (!stbi_write_png("split.0.png", dst_x, dst_y, n, top_left, dst_x * n)) {
    std::cerr << "Image write error: split.0.png" << std::endl;
  }

  if (!stbi_write_png("split.1.png", dst_x, dst_y, n, top_right, dst_x * n)) {
    std::cerr << "Image write error: split.1.png" << std::endl;
  }

  if (!stbi_write_png("split.2.png", dst_x, dst_y, n, bottom_left, dst_x * n)) {
    std::cerr << "Image write error: split.2.png" << std::endl;
  }

  if (!stbi_write_png("split.3.png", dst_x, dst_y, n, bottom_right, dst_x * n)) {
    std::cerr << "Image write error: split.3.png" << std::endl;
  }

  delete [] top_left;
  delete [] top_right;
  delete [] bottom_left;
  delete [] bottom_right;

  stbi_image_free(data);
  return 0;
}
