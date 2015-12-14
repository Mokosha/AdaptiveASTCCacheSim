#include "access_pattern.h"

#include <cassert>

#include "texture.h"

class RasterAccessPattern : public AccessPattern {
 protected:
  virtual std::vector<std::pair<int, int> >
  GenerateSamples(int w, int h) const {
    std::vector< std::pair<int, int> > samples;
    samples.reserve(w * h);
    for (int j = 0; j < h; ++j) {
      for (int i = 0; i < w; ++i) {
        samples.push_back(std::make_pair(i, j));
      }
    }

    return std::move(samples);
  }
};

static std::pair<int, int> Deinterleave(int x) {
  int x1 = 0, x2 = 0;
  for (size_t i = 0; i < sizeof(int)*8; ++i) {
    if ((i % 2) == 0) {
      x1 |= ((x & 0x1) << (i / 2)); 
    } else {
      x2 |= ((x & 0x1) << (i / 2)); 
    }
    x >>= 1;
  }
  return std::move(std::make_pair(x1, x2));
}

class MortonAccessPattern : public AccessPattern {
 protected:
  virtual std::vector<std::pair<int, int> >
  GenerateSamples(int w, int h) const {
    std::vector< std::pair<int, int> > samples;
    samples.reserve(w * h);
    for (int i = 0; i < w * h; ++i) {
      samples.push_back(std::move(Deinterleave(i)));
    }

    return std::move(samples);
  }
};

class RandomAccessPattern : public RasterAccessPattern {
 protected:
  virtual std::vector<std::pair<int, int> >
  GenerateSamples(int w, int h) const {
    std::vector< std::pair<int, int> > samples
      = std::move(RasterAccessPattern::GenerateSamples(w, h));

    // Shuffle
    for (size_t i = 0; i < samples.size() - 1; ++i) {
      size_t x = rand() % (samples.size() - 1 - i) + i;
      assert(x >= i);
      std::swap(samples[i], samples[x]);
    }

    return std::move(samples);
  }
};

std::unique_ptr<AccessPattern> AccessPattern::Create(EAccessPattern pattern) {
  switch(pattern) {
    case eAccessPattern_Random:
      return std::move(std::unique_ptr<AccessPattern>(new RandomAccessPattern));
    case eAccessPattern_Morton:
      return std::move(std::unique_ptr<AccessPattern>(new MortonAccessPattern));
    case eAccessPattern_Raster:
      return std::move(std::unique_ptr<AccessPattern>(new RasterAccessPattern));
  }
  assert(false);
  return nullptr;
}

void AccessPattern::Run(const std::unique_ptr<Texture> &tex, Cache *c) const {
  std::vector<std::pair<int, int> > samples =
    std::move(this->GenerateSamples(tex->GetWidth(), tex->GetHeight()));
  assert(samples.size() == static_cast<size_t>(tex->GetWidth() * tex->GetHeight()));

  for (auto sample : samples) {
    tex->Access(sample.first, sample.second, c);
  }
}
