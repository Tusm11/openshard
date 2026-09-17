#include "retrieval/retriever.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <algorithm>

namespace openshard {
namespace retrieval {

SelectiveRetriever::SelectiveRetriever(std::shared_ptr<index::Index> index)
    : index_(index) {}

std::vector<uint8_t> SelectiveRetriever::ExtractFromArchive(
    const std::string& archive_path,
    uint64_t offset,
    uint64_t size) {
  std::vector<uint8_t> result;
  
  std::ifstream archive(archive_path, std::ios::binary);
  if (!archive) {
    std::cerr << "Cannot open archive: " << archive_path << std::endl;
    return result;
  }

  archive.seekg(offset, std::ios::beg);
  result.resize(size);
  archive.read(reinterpret_cast<char*>(result.data()), size);
  archive.close();

  return result;
}

std::pair<std::vector<uint8_t>, RetrievalMetrics>
SelectiveRetriever::RetrieveFile(const std::string& filename) {
  auto start = std::chrono::high_resolution_clock::now();

  RetrievalMetrics metrics{0, 0, 0, false};
  std::vector<uint8_t> data;

  const auto* entry = index_->GetFile(filename);
  if (!entry) {
    std::cerr << "File not found in index: " << filename << std::endl;
    return {data, metrics};
  }

  data = ExtractFromArchive(entry->archive, entry->offset, entry->size);
  
  auto end = std::chrono::high_resolution_clock::now();
  metrics.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();
  metrics.bytes_requested = entry->size;
  metrics.bytes_transferred = data.size();
  metrics.success = (data.size() == entry->size);

  return {data, metrics};
}

std::vector<std::pair<std::vector<uint8_t>, RetrievalMetrics>>
SelectiveRetriever::RetrieveFiles(const std::vector<std::string>& filenames) {
  std::vector<std::pair<std::vector<uint8_t>, RetrievalMetrics>> results;
  for (const auto& filename : filenames) {
    results.push_back(RetrieveFile(filename));
  }
  return results;
}

}  // namespace retrieval
}  // namespace openshard
