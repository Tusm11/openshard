#pragma once

#include <string>
#include <vector>
#include <memory>
#include "index/index.h"

namespace openshard {
namespace retrieval {

struct RetrievalMetrics {
  uint64_t bytes_requested;
  uint64_t bytes_transferred;
  uint64_t latency_ms;
  bool success;
};

class SelectiveRetriever {
 public:
  explicit SelectiveRetriever(std::shared_ptr<index::Index> index);

  // Retrieve single file by name
  // Returns file contents + metrics
  std::pair<std::vector<uint8_t>, RetrievalMetrics> 
  RetrieveFile(const std::string& filename);

  // Retrieve multiple files
  std::vector<std::pair<std::vector<uint8_t>, RetrievalMetrics>>
  RetrieveFiles(const std::vector<std::string>& filenames);

 private:
  std::shared_ptr<index::Index> index_;

  // Extract file from archive at given offset/size
  std::vector<uint8_t> ExtractFromArchive(const std::string& archive_path,
                                         uint64_t offset,
                                         uint64_t size);
};

}  // namespace retrieval
}  // namespace openshard
