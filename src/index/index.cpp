#include "index/index.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace openshard {
namespace index {

void Index::AddFile(const std::string& filename,
                    const std::string& archive,
                    uint64_t offset,
                    uint64_t size) {
  entries_[filename] = {archive, offset, size};
}

const FileEntry* Index::GetFile(const std::string& filename) const {
  auto it = entries_.find(filename);
  if (it != entries_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::string Index::ToJson() const {
  // Simple CSV: filename,archive,offset,size
  std::string result;
  for (const auto& [name, entry] : entries_) {
    result += name + "," + entry.archive + "," + std::to_string(entry.offset) 
            + "," + std::to_string(entry.size) + "\n";
  }
  return result;
}

bool Index::FromJson(const std::string& data) {
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty()) continue;
    
    size_t pos1 = line.find(',');
    size_t pos2 = line.find(',', pos1 + 1);
    size_t pos3 = line.find(',', pos2 + 1);
    
    if (pos1 == std::string::npos || pos2 == std::string::npos || pos3 == std::string::npos) {
      return false;
    }
    
    std::string filename = line.substr(0, pos1);
    std::string archive = line.substr(pos1 + 1, pos2 - pos1 - 1);
    uint64_t offset = std::stoull(line.substr(pos2 + 1, pos3 - pos2 - 1));
    uint64_t size = std::stoull(line.substr(pos3 + 1));
    
    AddFile(filename, archive, offset, size);
  }
  return true;
}

bool Index::SaveToFile(const std::string& path) const {
  try {
    std::ofstream file(path);
    file << ToJson();
    file.close();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Save error: " << e.what() << std::endl;
    return false;
  }
}

bool Index::LoadFromFile(const std::string& path) {
  try {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return FromJson(buffer.str());
  } catch (const std::exception& e) {
    std::cerr << "Load error: " << e.what() << std::endl;
    return false;
  }
}

}  // namespace index
}  // namespace openshard
