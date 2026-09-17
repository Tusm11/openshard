// Archive reader abstraction not needed for Phase 10 - skip compilation
// Just use TarReader directly
#if 0
#include "archive/archive_reader.h"
#include "archive/tar_reader.h"
#include <fstream>
#include <iostream>

namespace openshard {
namespace archive {

std::unique_ptr<ArchiveReader> ArchiveReaderFactory::Create(
    const std::string& archive_path,
    ArchiveFormat format) {
  switch (format) {
    case ArchiveFormat::TAR:
      return std::make_unique<TarReader>(archive_path);
    case ArchiveFormat::ZIP:
      std::cerr << "ZIP format not yet implemented" << std::endl;
      return nullptr;
    case ArchiveFormat::GENERIC:
      return std::make_unique<TarReader>(archive_path);
  }
  return nullptr;
}

ArchiveFormat ArchiveReaderFactory::DetectFormat(
    const std::string& archive_path) {
  std::ifstream file(archive_path, std::ios::binary);
  if (!file) {
    return ArchiveFormat::GENERIC;
  }

  uint8_t magic[4];
  file.read(reinterpret_cast<char*>(magic), 4);
  file.close();

  if (magic[0] == 0x50 && magic[1] == 0x4B && magic[2] == 0x03 &&
      magic[3] == 0x04) {
    return ArchiveFormat::ZIP;
  }

  return ArchiveFormat::TAR;
}

}  // namespace archive
}  // namespace openshard
#endif

namespace openshard {
namespace archive {
// Stub implementation
ArchiveFormat ArchiveReaderFactory::DetectFormat(const std::string& path) {
  return ArchiveFormat::TAR;
}
}
}
