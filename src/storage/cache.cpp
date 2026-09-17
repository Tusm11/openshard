#include "storage/cache.h"
#include <algorithm>

namespace openshard {
namespace storage {

FileCache::FileCache(uint64_t max_size_bytes)
    : max_size_(max_size_bytes), current_size_(0), hits_(0), misses_(0) {}

const std::vector<uint8_t>* FileCache::Get(const std::string& key) {
  auto it = entries_.find(key);
  if (it != entries_.end()) {
    it->second.access_count++;
    it->second.timestamp = std::chrono::system_clock::now();
    hits_++;
    return &it->second.data;
  }
  misses_++;
  return nullptr;
}

void FileCache::Put(const std::string& key, const std::vector<uint8_t>& data) {
  uint64_t data_size = data.size();

  if (data_size > max_size_) {
    return;  // Don't cache if larger than entire cache
  }

  while (current_size_ + data_size > max_size_) {
    EvictLRU();
  }

  entries_[key] = {data, std::chrono::system_clock::now(), 1};
  current_size_ += data_size;
}

bool FileCache::Contains(const std::string& key) const {
  return entries_.count(key) > 0;
}

void FileCache::Clear() {
  entries_.clear();
  current_size_ = 0;
}

FileCache::Stats FileCache::GetStats() const {
  return {hits_, misses_, current_size_, entries_.size()};
}

void FileCache::EvictLRU() {
  if (entries_.empty()) return;

  auto lru = entries_.begin();
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    if (it->second.access_count < lru->second.access_count ||
        (it->second.access_count == lru->second.access_count &&
         it->second.timestamp < lru->second.timestamp)) {
      lru = it;
    }
  }

  current_size_ -= lru->second.data.size();
  entries_.erase(lru);
}

// RangeCache implementation
RangeCache::RangeCache(uint64_t max_size_bytes)
    : max_size_(max_size_bytes), current_size_(0) {}

const std::vector<uint8_t>* RangeCache::GetRange(const std::string& archive,
                                                 uint64_t offset,
                                                 uint64_t size) {
  RangeKey key{archive, offset, size};
  auto it = ranges_.find(key);
  if (it != ranges_.end()) {
    it->second.access_count++;
    it->second.timestamp = std::chrono::system_clock::now();
    return &it->second.data;
  }
  return nullptr;
}

void RangeCache::PutRange(const std::string& archive,
                         uint64_t offset,
                         const std::vector<uint8_t>& data) {
  RangeKey key{archive, offset, data.size()};

  while (current_size_ + data.size() > max_size_ && !ranges_.empty()) {
    auto lru = ranges_.begin();
    for (auto it = ranges_.begin(); it != ranges_.end(); ++it) {
      if (it->second.access_count < lru->second.access_count) {
        lru = it;
      }
    }
    current_size_ -= lru->second.data.size();
    ranges_.erase(lru);
  }

  ranges_[key] = {data, std::chrono::system_clock::now(), 1};
  current_size_ += data.size();
}

void RangeCache::EvictArchive(const std::string& archive) {
  for (auto it = ranges_.begin(); it != ranges_.end();) {
    if (it->first.archive == archive) {
      current_size_ -= it->second.data.size();
      it = ranges_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace storage
}  // namespace openshard
