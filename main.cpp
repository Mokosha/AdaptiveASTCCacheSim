#include <iostream>

#include <cassert>
#include <cstring>

#include "cache.h"
#include "texture.h"
#include "access_pattern.h"

static void PrintUsageAndExit() {
  std::cerr << "Usage: <<4x4|12x12> metadata_file vis_file | <ASTC4x4|ASTC8x8|ASTCx4|ASTC4x4> w h" << std::endl;
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
  } else if (strncmp(argv[1], "4x4", 3) == 0) {

    if (argc != 4) { PrintUsageAndExit(); }
    tex = Texture::Create(eTextureType_Adaptive4x4, argv[2], argv[3]);

  } else if (strncmp(argv[1], "12x12", 5) == 0) {

    if (argc != 4) { PrintUsageAndExit(); }
    tex = Texture::Create(eTextureType_Adaptive12x12, argv[2], argv[3]);

  } else {
    PrintUsageAndExit();
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
