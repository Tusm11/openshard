#include "retrieval/batch_retriever.h"
#include <algorithm>
#include <chrono>

namespace openshard {
namespace retrieval {

BatchRetriever::BatchRetriever(std::shared_ptr<index::Index> index)
    : index_(index), retriever_(std::make_shared<SelectiveRetriever>(index)) {}

BatchResult BatchRetriever::RetrieveFiles(const BatchRequest& request) {
  auto start = std::chrono::high_resolution_clock::now();

  BatchResult result{};
  result.success = true;

  for (const auto& filename : request.filenames) {
    auto [data, metrics] = retriever_->RetrieveFile(filename);
    result.metrics.push_back(metrics);
    result.total_bytes += metrics.bytes_transferred;

    if (!metrics.success) {
      result.success = false;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  result.total_latency_ms = 
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  return result;
}

uint64_t BatchRetriever::EstimateTotalSize(
    const std::vector<std::string>& filenames) const {
  uint64_t total = 0;
  for (const auto& filename : filenames) {
    const auto* entry = index_->GetFile(filename);
    if (entry) {
      total += entry->size;
    }
  }
  return total;
}

}  // namespace retrieval
}  // namespace openshard
