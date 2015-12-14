#include "texture.h"

#include <cassert>

#include "cache.h"

static const int kASTCBlockSize = 16;

class ASTCTexture : public Texture {
 public:
  ASTCTexture(int width, int height, int block_sz_x, int block_sz_y)
    : Texture(width, height)
    , _block_sz_x(block_sz_x)
    , _block_sz_y(block_sz_y)
    , _num_blocks_x((GetWidth() + block_sz_x - 1) / block_sz_x)
    , _num_blocks_y((GetWidth() + block_sz_y - 1) / block_sz_y)
  { }
  virtual ~ASTCTexture() { }

  virtual void Access(int x, int y, Cache *c) const {
    // Get the block index
    int block_x = x / _block_sz_x;
    int block_y = y / _block_sz_y;

    // The block offset
    int block_offset = block_y * _num_blocks_x + block_x;

    // The block address:
    int block_addr = block_offset * kASTCBlockSize;

    // Update cache...
    c->Access(block_addr);
  }

 private:
  int _block_sz_x;
  int _block_sz_y;

  int _num_blocks_x;
  int _num_blocks_y;
};

// !FIXME! Need to read metadata...

std::unique_ptr<Texture> Texture::Create(ETextureType type, int width, int height) {
  switch (type) {
    case eTextureType_ASTC4x4:
      return std::move(std::unique_ptr<Texture>(new ASTCTexture(width, height, 4, 4)));
    case eTextureType_ASTC6x6:
      return std::move(std::unique_ptr<Texture>(new ASTCTexture(width, height, 6, 6)));
    case eTextureType_ASTC8x8:
      return std::move(std::unique_ptr<Texture>(new ASTCTexture(width, height, 8, 8)));
    case eTextureType_ASTC12x12:
      return std::move(std::unique_ptr<Texture>(new ASTCTexture(width, height, 12, 12)));

    default:
      assert(false);
      // Do nothing
  }
  return nullptr;
}
