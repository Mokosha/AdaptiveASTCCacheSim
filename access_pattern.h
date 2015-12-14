#ifndef __ACCESS_PATTERN_H__
#define __ACCESS_PATTERN_H__

#include <memory>
#include <vector>

enum EAccessPattern {
  eAccessPattern_Random,
  eAccessPattern_Morton,
  eAccessPattern_Raster,
};

// Forward declare
class Texture;
class Cache;

class AccessPattern {
 public:
  static std::unique_ptr<AccessPattern> Create(EAccessPattern pattern);
  void Run(const std::unique_ptr<Texture> &tex, Cache *c) const;

 protected:
  AccessPattern() { }

  virtual std::vector<std::pair<int, int> >
    GenerateSamples(int w, int h) const = 0;
};

#endif // __ACCESS_PATTERN_H__
