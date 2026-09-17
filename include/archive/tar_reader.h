#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <map>

namespace openshard {
namespace archive {

struct ArchiveFile {
  std::string name;
  uint64_t size;
  uint64_t offset;
};

class TarReader {
 public:
  explicit TarReader(const std::string& tar_path);
  ~TarReader();

  // Scan TAR and build index
  bool Index();

  // Get file info by name
  const ArchiveFile* GetFile(const std::string& name) const;

  // List all files
  const std::vector<ArchiveFile>& Files() const { return files_; }

  // Get index size (metadata only)
  size_t IndexSize() const { return files_.size(); }

 private:
  std::string tar_path_;
  std::vector<ArchiveFile> files_;
  std::map<std::string, size_t> file_index_;

  // Parse single TAR header block (512 bytes)
  ArchiveFile ParseHeader(const uint8_t* header, uint64_t offset_in_archive);
};

}  // namespace archive
}  // namespace openshard
