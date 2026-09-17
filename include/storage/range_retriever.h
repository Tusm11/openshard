#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace openshard {
namespace storage {

struct RangeRetrievalResult {
  uint64_t offset;
  uint64_t size;
  uint64_t bytes_received;
  std::chrono::milliseconds latency;
  bool success;
  std::string error;
};

class RangeRetriever {
 public:
  RangeRetriever(const std::string& endpoint);
  ~RangeRetriever();

  // Retrieve a byte range from remote object
  RangeRetrievalResult GetRange(const std::string& object_key,
                               uint64_t offset,
                               uint64_t size);

  // Get object size without downloading
  uint64_t GetObjectSize(const std::string& object_key);

 private:
  std::string endpoint_;
};

}  // namespace storage
}  // namespace openshard
