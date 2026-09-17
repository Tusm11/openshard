#include "index/index_builder.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace openshard {
namespace index {

bool IndexBuilder::BuildFromTar(const std::string& tar_path, 
                                std::shared_ptr<Index> index) {
  file_count_ = 0;

  std::ifstream tar(tar_path, std::ios::binary);
  if (!tar) {
    std::cerr << "Failed to open TAR: " << tar_path << std::endl;
    return false;
  }

  uint64_t file_offset = 0;
  uint8_t header[512];

  while (tar.read(reinterpret_cast<char*>(header), 512)) {
    // Check for end marker (two zero blocks)
    if (std::all_of(header, header + 512, [](uint8_t c) { return c == 0; })) {
      break;
    }

    // Parse TAR header
    char name[101] = {0};
    std::memcpy(name, header, 100);
    std::string filename = std::string(name);
    filename.erase(filename.find_last_not_of("\0 ") + 1);

    // Parse size (octal at offset 124, 12 bytes)
    char size_str[13] = {0};
    std::memcpy(size_str, header + 124, 12);
    uint64_t file_size = std::stoull(size_str, nullptr, 8);

    if (!filename.empty() && filename[0] != '\0') {
      // File data starts after the 512-byte header
      uint64_t data_offset = file_offset + 512;

      // Add to index
      index->AddFile(filename, tar_path, data_offset, file_size);
      file_count_++;

      // Calculate next file position
      // Data is padded to 512-byte boundary
      uint64_t padded_size = ((file_size + 511) / 512) * 512;
      file_offset = data_offset + padded_size;

      // Seek to next header
      tar.seekg(padded_size, std::ios::cur);
    }
  }

  tar.close();
  return true;
}

bool IndexBuilder::BuildAndSave(const std::string& tar_path, 
                                const std::string& index_path) {
  auto index = std::make_shared<Index>();

  if (!BuildFromTar(tar_path, index)) {
    return false;
  }

  return index->SaveToFile(index_path);
}

}  // namespace index
}  // namespace openshard
