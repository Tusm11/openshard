#include "archive/tar_reader.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace openshard {
namespace archive {

TarReader::TarReader(const std::string& tar_path) : tar_path_(tar_path) {}

TarReader::~TarReader() = default;

ArchiveFile TarReader::ParseHeader(const uint8_t* header, uint64_t offset_in_archive) {
  ArchiveFile file;
  file.offset = offset_in_archive;

  // TAR header: filename at offset 0, 100 bytes
  char name[101] = {0};
  std::memcpy(name, header, 100);
  file.name = std::string(name);
  // Remove trailing nulls/spaces
  file.name.erase(file.name.find_last_not_of("\0 ") + 1);

  // Size at offset 124, 12 bytes (octal)
  char size_str[13] = {0};
  std::memcpy(size_str, header + 124, 12);
  file.size = std::stoull(size_str, nullptr, 8);

  return file;
}

bool TarReader::Index() {
  std::ifstream tar(tar_path_, std::ios::binary);
  if (!tar) {
    std::cerr << "Failed to open TAR: " << tar_path_ << std::endl;
    return false;
  }

  uint64_t file_offset = 0;
  uint8_t header[512];

  while (tar.read(reinterpret_cast<char*>(header), 512)) {
    // Check for end marker (two zero blocks)
    if (std::all_of(header, header + 512, [](uint8_t c) { return c == 0; })) {
      break;
    }

    ArchiveFile file = ParseHeader(header, file_offset);
    
    if (!file.name.empty() && file.name[0] != '\0') {
      files_.push_back(file);
      file_index_[file.name] = files_.size() - 1;

      // Advance by header (512) + padded content
      uint64_t padded_size = ((file.size + 511) / 512) * 512;
      file_offset += 512 + padded_size;
      tar.seekg(padded_size, std::ios::cur);
    }
  }

  tar.close();
  return true;
}

const ArchiveFile* TarReader::GetFile(const std::string& name) const {
  auto it = file_index_.find(name);
  if (it != file_index_.end()) {
    return &files_[it->second];
  }
  return nullptr;
}

}  // namespace archive
}  // namespace openshard
