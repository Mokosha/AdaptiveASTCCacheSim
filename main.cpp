#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cache.h"
#include "texture.h"
#include "access_pattern.h"

static void PrintUsageAndExit() {
  std::cerr << "Usage: <metadata_file | <ASTC4x4|ASTC8x8|ASTCx4|ASTC4x4> w h" << std::endl;
  exit(1);
}

int main(int argc, char **argv) {
  if (argc == 1) { PrintUsageAndExit(); }

  std::unique_ptr<Texture> tex = nullptr;
  if (strncmp(argv[1], "ASTC", 4) == 0) {

    if (argc != 4) { PrintUsageAndExit(); }

    int w = atoi(argv[2]);
    int h = atoi(argv[3]);

    if(std::string(argv[1]) == std::string("ASTC4x4")) {
      tex = Texture::Create(eTextureType_ASTC4x4, w, h);
    } else if(std::string(argv[1]) == std::string("ASTC8x8")) {
      tex = Texture::Create(eTextureType_ASTC8x8, w, h);
    } else if(std::string(argv[1]) == std::string("ASTC6x6")) {
      tex = Texture::Create(eTextureType_ASTC6x6, w, h);
    } else if(std::string(argv[1]) == std::string("ASTC12x12")) {
      tex = Texture::Create(eTextureType_ASTC12x12, w, h);
    }
  } else {

    if (argc != 2) { PrintUsageAndExit(); }

    // Parse metadata file...
    // Build metadata
    // std::unique_ptr<Metadata> = Metadata::Create(metadata_type);
  }

  // 1KB cache...
  Cache c(1);

  // Generate three different access patterns...
  std::unique_ptr<AccessPattern> ap = AccessPattern::Create(eAccessPattern_Random);
  ap->Run(tex, &c);
  std::cout << "Cache stats for random access pattern: " << std::endl;
  c.PrintStats();
  std::cout << std::endl;
  c.Clear();

  ap = AccessPattern::Create(eAccessPattern_Morton);
  ap->Run(tex, &c);
  std::cout << "Cache stats for morton access pattern: " << std::endl;
  c.PrintStats();
  std::cout << std::endl;
  c.Clear();

  ap = AccessPattern::Create(eAccessPattern_Raster);
  ap->Run(tex, &c);
  std::cout << "Cache stats for raster access pattern: " << std::endl;
  c.PrintStats();
  std::cout << std::endl;
  c.Clear();

  return 1;
}
