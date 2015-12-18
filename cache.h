#ifndef __CACHE_H__
#define __CACHE_H__

#include <vector>
#include <iostream>

struct CacheStats {
  size_t num_hits;
  size_t num_misses;
  size_t num_accesses;
};

// LRU cache with 64 byte cache lines.
class Cache {
 public:
  Cache(size_t size_in_kb)
    : _size_in_kb(size_in_kb)
    , _lines([&size_in_kb] {
        int num_lines = (size_in_kb * 1024) / 64;
        CacheEntry entry;
        entry._time = 0;
        entry._addr = 0;
        entry._valid = false;
        std::vector<CacheEntry> result =
          std::vector<CacheEntry>(num_lines, entry);
        return std::move(result);
      }())
    , _num_hits(0)
    , _num_misses(0)
    , _num_accesses(0)
    , _time_point(0)
    { }

  void Access(size_t address, size_t num_bytes) {
    if (num_bytes == 1) {
      Access(address);
    } else if (num_bytes > 1) {
      size_t end_address = address + num_bytes - 1;

      // Align the two addresses to 64 byte boundary...
      address = (address >> 6) << 6;
      end_address = (end_address >> 6) << 6;

      Access(address);
      while (address != end_address) {
        assert(end_address > address);
        assert((end_address - address) % 64 == 0);

        Access(address);
        address += 64;
      }
    }
  }

  void Access(size_t address) {
    _time_point++;
    _num_accesses++;

    // Align the address to a 64 byte boundary
    address = (address >> 6) << 6;

    // Search for matching cache lines...
    CacheEntry *lru = NULL;
    for (auto it = _lines.begin(); it != _lines.end(); ++it) {
      CacheEntry &entry = *it;
      if (entry._valid) {
        if (entry._addr == address) {
          _num_hits++;
          entry._time = _time_point;
          return;
        }

        if (nullptr == lru) {
          // First entry we've encountered?
          lru = &entry;
        } else if (lru->_valid && entry._time < lru->_time) {
          // LRU is valid and this one is worse
          lru = &entry;
        }
      } else {
        // Entry is invalid -- kick this one out first.
        lru = &entry;
      }
    }

    // OK, cache miss -- change the cache entry
    _num_misses++;
    lru->_addr = address;
    lru->_time = _time_point;
    lru->_valid = true;
  }

  void PrintStats() {
    std::cout << "Num cache hits: " << _num_hits << std::endl;
    std::cout << "Num cache misses: " << _num_misses << std::endl;
    std::cout << "Num cache accesses: " << _num_accesses << std::endl;
  }

  void Clear() {
    for (auto &entry : _lines) {
      entry._valid = false;
    }

    _num_hits = _num_misses = _num_accesses = _time_point = 0;
  }

 private:
  struct CacheEntry {
    size_t _addr;
    size_t _time;
    bool _valid;
  };

  const int _size_in_kb;
  std::vector<CacheEntry> _lines;
  size_t _num_hits;
  size_t _num_misses;
  size_t _num_accesses;
  size_t _time_point;
};

#endif  // __CACHE_H__
