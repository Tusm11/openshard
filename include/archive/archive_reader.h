#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace openshard {
namespace archive {

enum class ArchiveFormat {
  TAR,
  ZIP,
  GENERIC,
};

struct ArchiveFile {
  std::string name;
  uint64_t size;
  uint64_t offset;
};

class ArchiveReader {
 public:
  virtual ~ArchiveReader() = default;

  // Scan archive and index files
  virtual bool Index() = 0;

  // Get file by name
  virtual const ArchiveFile* GetFile(const std::string& name) const = 0;

  // List all files
  virtual const std::vector<ArchiveFile>& Files() const = 0;

  // Get format
  virtual ArchiveFormat Format() const = 0;
};

// Factory for creating readers
class ArchiveReaderFactory {
 public:
  static std::unique_ptr<ArchiveReader> Create(const std::string& archive_path,
                                               ArchiveFormat format);

  static ArchiveFormat DetectFormat(const std::string& archive_path);
};

}  // namespace archive
}  // namespace openshard
