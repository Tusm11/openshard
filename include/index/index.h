#pragma once

#include <string>
#include <map>
#include <cstdint>

namespace openshard {
namespace index {

struct FileEntry {
  std::string archive;
  uint64_t offset;
  uint64_t size;
};

class Index {
 public:
  Index() = default;

  // Add file entry
  void AddFile(const std::string& filename,
               const std::string& archive,
               uint64_t offset,
               uint64_t size);

  // Get file entry
  const FileEntry* GetFile(const std::string& filename) const;

  // Serialize to string (simple CSV format)
  std::string ToJson() const;

  // Deserialize from string
  bool FromJson(const std::string& data);

  // Save to file
  bool SaveToFile(const std::string& path) const;

  // Load from file
  bool LoadFromFile(const std::string& path);

  // Get all entries
  const std::map<std::string, FileEntry>& Entries() const { return entries_; }

 private:
  std::map<std::string, FileEntry> entries_;
};

}  // namespace index
}  // namespace openshard
