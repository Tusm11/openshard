#pragma once

#include <string>
#include <memory>
#include "index.h"

namespace openshard {
namespace index {

class IndexBuilder {
 public:
  IndexBuilder() = default;

  // Build index from TAR archive
  bool BuildFromTar(const std::string& tar_path, std::shared_ptr<Index> index);

  // Build index and save to CSV
  bool BuildAndSave(const std::string& tar_path, const std::string& index_path);

  // Get count of files indexed
  size_t GetFileCount() const { return file_count_; }

 private:
  size_t file_count_;
};

}  // namespace index
}  // namespace openshard
