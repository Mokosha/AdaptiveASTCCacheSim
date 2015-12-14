#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <memory>

enum ETextureType {
  eTextureType_ASTC4x4,
  eTextureType_ASTC6x6,
  eTextureType_ASTC8x8,
  eTextureType_ASTC12x12,

  eTextureType_Adaptive4x4,
  eTextureType_Adaptive12x12
};

// Forward declare...
class Cache;

class Texture {
 public:
  static std::unique_ptr<Texture> Create(ETextureType type, int width, int height);
  virtual ~Texture() { }

  virtual void Access(int x, int y, Cache *c) const = 0;

  int GetWidth() const { return _w; }
  int GetHeight() const { return _h; }

 protected:
  Texture(int width, int height) : _w(width), _h(height) { }

 private:
  Texture();
  int _w;
  int _h;
};

#endif  // __TEXTURE_H__
