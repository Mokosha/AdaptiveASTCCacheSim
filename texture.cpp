#include "texture.h"

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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
    c->Access(block_addr, 16);
  }

 private:
  const int _block_sz_x;
  const int _block_sz_y;

  const int _num_blocks_x;
  const int _num_blocks_y;
};

class Metadata4x4Texture : public Texture {
 public:
  Metadata4x4Texture(int width, int height, const std::unordered_map<int, int> &duplicates,
                     int num_channels, const unsigned char *vis_image_data)
    : Texture(width, height)
    , _next_block_idx(0)
    , _num_blocks_x((width + 3) / 4)
    , _num_blocks_y((height + 3) / 4)
    , _metadata(_num_blocks_x * _num_blocks_y, MetadataEntry()) {

    UpdateImage<12>(width, height, 0xFF0000FF, vis_image_data, num_channels, eBlockType_12x12_0);
    UpdateImage<8>(width, height, 0xFFFF0000, vis_image_data, num_channels, eBlockType_8x8_0);

    // Every remaining block is a 4x4 block...
    for (auto &entry : _metadata) {
      if (entry.GetBlockOffset() < 0) {
        entry.SetBlockOffset(_next_block_idx++);
        entry.SetBlockType(eBlockType_4x4);
      }
    }

    // Look to see if any of the blocks are similar.
    const int num_blocks = _num_blocks_x * _num_blocks_y;
    int next_block_idx = 0;
    for (int i = 0; i < num_blocks; ++i) {
      int same_idx = duplicates.at(i);
      if (same_idx != next_block_idx) {
        _metadata[i] = _metadata[same_idx];
      } else {
        next_block_idx++;
      }
    }

    // Find any duplicates...
    std::vector<bool> used(num_blocks, false);
    for (const auto &entry : _metadata) {
      int offset = entry.GetBlockOffset();
      if (offset >= 0) {
        used[offset] = true;
      }
    }

    std::vector<size_t> unused;
    unused.reserve(num_blocks);
    for (size_t i = 0; i < used.size(); ++i) {
      if (!used[i]) {
        unused.push_back(i);
      }
    }

    // Fix offsets
    if (unused.size() > 0) {
      for (auto &entry : _metadata) {
        size_t offset = static_cast<size_t>(entry.GetBlockOffset());

        // Binary search for first unused greater than
        // the current offset...
        int low = 0, high = unused.size() - 1;
        while (high - low > 1) {
          int mid = (low + high) >> 1;
          if (unused[mid] > offset) { high = mid; }
          else if (unused[mid] < offset) { low = mid; }
          else { low = high = mid; }
        }

        entry.SetBlockOffset(offset - low);
      }
    }
  }

  virtual ~Metadata4x4Texture() { }

  virtual void Access(int x, int y, Cache *c) const {
    // Get the block index
    int block_x = x / 4;
    int block_y = y / 4;

    // The block offset
    int block_idx = block_y * _num_blocks_x + block_x;

    // Lookup offset in metadata
    c->Access(block_idx * 3, 3);
    const MetadataEntry &entry = _metadata[block_idx];
    int offset = entry.GetBlockOffset();

    // The block address:
    int block_addr = 3 * _metadata.size() + offset * kASTCBlockSize;

    // Update cache...
    c->Access(block_addr, 16);
  }

 private:

  enum EBlockType {
    eBlockType_4x4,
    eBlockType_8x8_0,
    eBlockType_8x8_1,
    eBlockType_8x8_2,
    eBlockType_8x8_3,
    eBlockType_12x12_0,
    eBlockType_12x12_1,
    eBlockType_12x12_2,
    eBlockType_12x12_3,
    eBlockType_12x12_4,
    eBlockType_12x12_5,
    eBlockType_12x12_6,
    eBlockType_12x12_7,
    eBlockType_12x12_8,
    eBlockType_12x12_9,
  };

  class MetadataEntry {
   public:
    MetadataEntry() : _offset(-1), _type(eBlockType_4x4) { }

    int GetBlockOffset() const { return _offset; }
    void SetBlockOffset(int offset) { _offset = offset; }

    EBlockType GetBlockType() const { return _type; }
    void SetBlockType(EBlockType ty) { _type = ty; }

   private:
    int _offset;
    EBlockType _type;
  };

  template<unsigned kBlockSize>
  void UpdateImage(int width, int height, int color, const unsigned char *vis_image_data,
                   int num_channels, EBlockType first_type) {
    assert(kBlockSize % 4 == 0);
    const int k4x4BlocksPerBlock = kBlockSize / 4;

    // Go through the vis image in incremental block sizes...
    for (int j = 0; j < height; j += kBlockSize) {
      for (int i = 0; i < width; i += kBlockSize) {

        int good_blocks[k4x4BlocksPerBlock * k4x4BlocksPerBlock];
        memset(good_blocks, 0xFF, sizeof(good_blocks));

        // Check to see if any of the blocks here are red...
        for (int offset_y = 0; offset_y < k4x4BlocksPerBlock; ++offset_y) {
          for (int offset_x = 0; offset_x < k4x4BlocksPerBlock; ++offset_x) {

            const unsigned char *block_offset_data = vis_image_data;
            block_offset_data += ((j + offset_y * 4) * width + i + offset_x * 4) * num_channels;

            bool ok = true;
            for (int y = 0; y < 4; ++y) {
              for (int x = 0; x < 4; ++x) {
                ok = ok && 0 == memcmp(block_offset_data + (y * width + x) * num_channels,
                                       &color, num_channels);
              }
            }

            int block_idx_x = (i / 4) + offset_x;
            int block_idx_y = (j / 4) + offset_y;
            int block_idx = block_idx_y * _num_blocks_x + block_idx_x;

            int offset_idx = offset_y * k4x4BlocksPerBlock + offset_x;

            if (ok) {
              good_blocks[offset_idx] = block_idx;
            }
          }
        }

        bool any_blocks = false;
        for (int b = 0; b < k4x4BlocksPerBlock * k4x4BlocksPerBlock; ++b) {
          int idx = good_blocks[b];
          if (idx >= 0) {
            _metadata[idx].SetBlockOffset(_next_block_idx);
            _metadata[idx].SetBlockType(
                static_cast<EBlockType>(static_cast<int>(first_type) + b));

            any_blocks = true;
          }
        }

        if (any_blocks) {
          _next_block_idx++;
        }
      }
    }
  }

  int _next_block_idx;
  const int _num_blocks_x;
  const int _num_blocks_y;
  std::vector<MetadataEntry> _metadata;
};

class Metadata12x12Texture : public Texture {
 public:
  Metadata12x12Texture(int width, int height, const std::unordered_map<int, int> &duplicates,
                       int num_channels, const unsigned char *vis_image_data)
    : Texture(width, height)
    , _next_block_idx(0)
    , _num_blocks_x((width + 11) / 12)
    , _num_blocks_y((height + 11) / 12)
    , _metadata(_num_blocks_x * _num_blocks_y, MetadataEntry()) {

    int blocks_written = 0;
    int next_block_idx = 0;
    int block_idx = 0;
    for (int j = 0; j < height; j += 12) {
      for (int i = 0; i < width; i += 12) {

        // If we've already visited this block, then just copy it over...
        if (duplicates.at(block_idx) != next_block_idx) {
          _metadata[block_idx] = _metadata[duplicates.at(block_idx)];
        } else {
          next_block_idx++;

          // Figure out what kind of block this is...
          size_t offset = (j * width + i) * num_channels;
          MetadataEntry &e = _metadata[block_idx];
          e.SetBlockType(AnalyzeBlock(vis_image_data + offset, width * num_channels, num_channels));
          e.SetBlockOffset(blocks_written);
          blocks_written += e.GetBlocksToRead();
        }

        block_idx++;
      }
    }

    assert(block_idx == _num_blocks_x * _num_blocks_y);
  }

  virtual ~Metadata12x12Texture() { }

  virtual void Access(int x, int y, Cache *c) const {
    // Get the block index
    int block_x = x / 12;
    int block_y = y / 12;

    // The block offset
    int block_idx = block_y * _num_blocks_x + block_x;

    // Lookup offset in metadata
    c->Access(block_idx * 3, 3);
    const MetadataEntry &entry = _metadata[block_idx];
    int offset = entry.GetBlockOffset();

    // The block address:
    int block_addr = 3 * _metadata.size() + offset * kASTCBlockSize;

    // Update cache...
    c->Access(block_addr, entry.GetBlocksToRead() * 16);
  }

 private:

  static const uint32_t kRed = 0xFF0000FF;
  static const uint32_t kGreen = 0xFF00FF00;
  static const uint32_t kBlue = 0xFFFF0000;
  static const uint32_t kYellow = 0xFF00FFFF;

  static uint32_t PixelAt(const unsigned char *data, size_t rowbytes, size_t num_channels, int i, int j) {
    const unsigned char *offset_data = data + j * rowbytes + i * num_channels;
    uint32_t pixel = *(reinterpret_cast<const uint32_t *>(offset_data));
    if (3 == num_channels) {
      pixel &= 0x00FFFFFF;
      pixel |= 0xFF000000;
    }
    return pixel;
  }

  size_t IsAllOneColor(const unsigned char *data, size_t rowbytes, size_t num_channels) {
    uint32_t first_pixel = PixelAt(data, rowbytes, num_channels, 0, 0);
    switch(first_pixel) {
      case kRed:
      case kGreen:
      case kBlue:
        break;
      default:
        return 0;
    }

    bool ok = true;
    for (int j = 0; j < 12; j++) {
      for (int i = 0; i < 12; i++) {
        ok = ok && PixelAt(data, rowbytes, num_channels, i, j) == first_pixel;
      }
    }

    if (ok) {
      return first_pixel;
    } else {
      return 0;
    }
  }

  int Is8x8(const unsigned char *data, size_t rowbytes, size_t num_channels) {
    int mode = -1;
    if (PixelAt(data, rowbytes, num_channels, 0, 0) == kYellow) {
      mode = 0;
    } else if (PixelAt(data, rowbytes, num_channels, 4, 0) == kYellow) {
      mode = 1;
    } else if (PixelAt(data, rowbytes, num_channels, 0, 4) == kYellow) {
      mode = 2;
    } else if (PixelAt(data, rowbytes, num_channels, 4, 4) == kYellow) {
      mode = 3;
    }

    int offset_x, offset_y;
    switch (mode) {
      case 0:
        offset_x = 0;
        offset_y = 0;
        break;
      case 1:
        offset_x = 4;
        offset_y = 0;
        break;
      case 2:
        offset_x = 0;
        offset_y = 4;
        break;
      case 3:
        offset_x = 4;
        offset_y = 4;
        break;
      default: return -1;
    }

    bool ok = true;
    for (int j = 0; j < 12; ++j) {
      for (int i = 0; i < 12; ++i) {
        uint32_t color = kYellow;

        int di = i - offset_x;
        if (di < 0 || di > 8) {
          color = kGreen;
        }

        int dj = j - offset_y;
        if (dj < 0 || dj > 8) {
          color = kGreen;
        }

        ok = ok && PixelAt(data, rowbytes, num_channels, i, j) == color;
      }
    }

    return mode;
  }

  enum EBlockType {
    eBlockType_4x4,
    eBlockType_6x6,
    eBlockType_8x8_0,
    eBlockType_8x8_1,
    eBlockType_8x8_2,
    eBlockType_8x8_3,
    eBlockType_12x12,
  };

  EBlockType AnalyzeBlock(const unsigned char *data, size_t rowbytes, size_t num_channels) {
    uint32_t color = IsAllOneColor(data, rowbytes, num_channels);
    switch(color) {
      case kRed: return eBlockType_12x12;
      case kGreen: return eBlockType_4x4;
      case kBlue: return eBlockType_6x6;
      default:
        break;
    }

    int is8x8 = Is8x8(data, rowbytes, num_channels);
    if (is8x8 >= 0) {
      assert(is8x8 < 4);
      return static_cast<EBlockType>(static_cast<int>(eBlockType_8x8_0) + is8x8);
    }

    assert(false);
    return eBlockType_4x4;
  }

  class MetadataEntry {
   public:
    MetadataEntry() : _offset(-1), _type(eBlockType_4x4) { }

    int GetBlockOffset() const { return _offset; }
    void SetBlockOffset(int offset) { _offset = offset; }

    EBlockType GetBlockType() const { return _type; }
    void SetBlockType(EBlockType ty) { _type = ty; }

    int GetBlocksToRead() const {
      switch(_type) {
      case eBlockType_4x4:
        return 9;
      case eBlockType_6x6:
        return 4;
      case eBlockType_8x8_0:
      case eBlockType_8x8_1:
      case eBlockType_8x8_2:
      case eBlockType_8x8_3:
        return 6;
      case eBlockType_12x12:
        return 1;
      }
      assert(false);
      return 0;
    }

   private:
    int _offset;
    EBlockType _type;
  };

  int _next_block_idx;
  const int _num_blocks_x;
  const int _num_blocks_y;
  std::vector<MetadataEntry> _metadata;
};

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


std::unique_ptr<Texture> Texture::Create(ETextureType type,
                                         const char *metadata_filename,
                                         const char *vis_filename) {
  // Parse the metadata...
  std::unordered_map<int, int> duplicates;
  std::ifstream dup_file(metadata_filename);
  // Skip the first line
  dup_file.ignore(1024, '\n');

  int idx;
  int i = 0;
  while (dup_file >> idx) {
    duplicates[i++] = idx;
  }

  int w = 0, h = 0, channels = 0;
  unsigned char *data = stbi_load(vis_filename, &w, &h, &channels, 0);
  if (!data) {
    std::cerr << "Error loading image: " << vis_filename << std::endl;
    exit(1);
  }

  // Clamp width and height appropriately...
  w = (w / 12) * 12;
  h = (h / 12) * 12;

  switch (type) {
  case eTextureType_Adaptive4x4:
    return std::move(std::unique_ptr<Texture>(new Metadata4x4Texture(w, h, duplicates, channels, data)));
  case eTextureType_Adaptive12x12:
    return std::move(std::unique_ptr<Texture>(new Metadata12x12Texture(w, h, duplicates, channels, data)));
  default:
    assert(false);
  }

  return nullptr;
}
