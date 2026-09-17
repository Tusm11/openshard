#pragma once

#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <memory>

namespace openshard {
namespace storage {

struct CacheEntry {
  std::vector<uint8_t> data;
  std::chrono::system_clock::time_point timestamp;
  uint64_t access_count;
};

class FileCache {
 public:
  FileCache(uint64_t max_size_bytes);

  // Get cached file
  const std::vector<uint8_t>* Get(const std::string& key);

  // Store file
  void Put(const std::string& key, const std::vector<uint8_t>& data);

  // Check if cached
  bool Contains(const std::string& key) const;

  // Clear cache
  void Clear();

  // Get statistics
  struct Stats {
    uint64_t hits;
    uint64_t misses;
    uint64_t current_size;
    size_t entry_count;
  };
  Stats GetStats() const;

 private:
  uint64_t max_size_;
  uint64_t current_size_;
  uint64_t hits_;
  uint64_t misses_;
  std::map<std::string, CacheEntry> entries_;

  void EvictLRU();
};

class RangeCache {
 public:
  RangeCache(uint64_t max_size_bytes);

  // Get range
  const std::vector<uint8_t>* GetRange(const std::string& archive,
                                       uint64_t offset,
                                       uint64_t size);

  // Store range
  void PutRange(const std::string& archive,
               uint64_t offset,
               const std::vector<uint8_t>& data);

  // Evict ranges for archive
  void EvictArchive(const std::string& archive);

 private:
  uint64_t max_size_;
  uint64_t current_size_;
  struct RangeKey {
    std::string archive;
    uint64_t offset;
    uint64_t size;

    bool operator<(const RangeKey& other) const {
      if (archive != other.archive) return archive < other.archive;
      return offset < other.offset;
    }
  };
  std::map<RangeKey, CacheEntry> ranges_;
};

}  // namespace storage
}  // namespace openshard
