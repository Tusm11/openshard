#pragma once

#include <vector>
#include <string>
#include <memory>
#include "retriever.h"
#include "index/index.h"

namespace openshard {
namespace retrieval {

struct BatchRequest {
  std::vector<std::string> filenames;
};

struct BatchResult {
  std::vector<RetrievalMetrics> metrics;
  uint64_t total_bytes;
  uint64_t total_latency_ms;
  bool success;
};

class BatchRetriever {
 public:
  explicit BatchRetriever(std::shared_ptr<index::Index> index);

  // Retrieve multiple files
  BatchResult RetrieveFiles(const BatchRequest& request);

  // Estimate total size before retrieval
  uint64_t EstimateTotalSize(const std::vector<std::string>& filenames) const;

 private:
  std::shared_ptr<SelectiveRetriever> retriever_;
  std::shared_ptr<index::Index> index_;
};

}  // namespace retrieval
}  // namespace openshard
