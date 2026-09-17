#include "index/index_builder.h"
#include "archive/tar_reader.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>

using namespace openshard;

// Create a minimal TAR with 3 files
void CreateTestTar(const std::string& path) {
  std::ofstream tar(path, std::ios::binary);

  auto write_file = [&tar](const std::string& filename, const std::string& content) {
    // Create header block (512 bytes)
    uint8_t header[512] = {0};
    
    // Filename at offset 0
    std::strcpy((char*)header, filename.c_str());
    
    // Size at offset 124 (octal, null-terminated, right-aligned)
    // Convert size to octal string
    uint64_t size = content.size();
    char size_buf[12];
    snprintf(size_buf, sizeof(size_buf), "%011llo", (unsigned long long)size);
    
    std::memcpy(header + 124, size_buf, 11);
    header[124 + 11] = '\0';  // Null-terminate

    // Write header
    tar.write((char*)header, 512);
    
    // Write content
    tar.write(content.c_str(), content.size());
    
    // Write padding to 512-byte boundary
    uint64_t padded_size = ((content.size() + 511) / 512) * 512;
    uint64_t padding_needed = padded_size - content.size();
    if (padding_needed > 0) {
      uint8_t padding[512] = {0};
      tar.write((char*)padding, padding_needed);
    }
  };

  // Write three test files
  write_file("a.pdf", "PDF file A content");
  write_file("b.pdf", "This is PDF file B with more content");
  write_file("c.txt", "Text file C");

  // Write end markers (two zero blocks)
  uint8_t zero_block[512] = {0};
  tar.write((char*)zero_block, 512);
  tar.write((char*)zero_block, 512);

  tar.close();
}

int main() {
  try {
    std::cout << "=== OpenShard Index Builder Test ===" << std::endl;

    // Create test TAR
    std::cout << "\n1. Creating test TAR..." << std::endl;
    CreateTestTar("test_index.tar");
    std::cout << "   ✓ test_index.tar created" << std::endl;

    // Build index
    std::cout << "\n2. Building index..." << std::endl;
    index::IndexBuilder builder;
    auto index = std::make_shared<index::Index>();
    
    if (!builder.BuildFromTar("test_index.tar", index)) {
      std::cerr << "   ✗ Failed to build index" << std::endl;
      return 1;
    }

    std::cout << "   ✓ Index built: " << builder.GetFileCount() << " files" << std::endl;

    // Verify entries
    std::cout << "\n3. Verifying index entries..." << std::endl;
    
    const auto* a = index->GetFile("a.pdf");
    const auto* b = index->GetFile("b.pdf");
    const auto* c = index->GetFile("c.txt");

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);

    std::cout << "   a.pdf: offset=" << a->offset << " size=" << a->size << std::endl;
    std::cout << "   b.pdf: offset=" << b->offset << " size=" << b->size << std::endl;
    std::cout << "   c.txt: offset=" << c->offset << " size=" << c->size << std::endl;

    // Verify sizes
    assert(a->size == 18);  // "PDF file A content"
    assert(b->size == 36);  // "This is PDF file B with more content"
    assert(c->size == 11);  // "Text file C"

    std::cout << "   ✓ All entries verified" << std::endl;

    // Save index
    std::cout << "\n4. Saving index..." << std::endl;
    if (!index->SaveToFile("test_index.csv")) {
      std::cerr << "   ✗ Failed to save index" << std::endl;
      return 1;
    }
    std::cout << "   ✓ test_index.csv saved" << std::endl;

    // Load and verify
    std::cout << "\n5. Loading index back..." << std::endl;
    auto loaded_index = std::make_shared<index::Index>();
    if (!loaded_index->LoadFromFile("test_index.csv")) {
      std::cerr << "   ✗ Failed to load index" << std::endl;
      return 1;
    }

    const auto* loaded_a = loaded_index->GetFile("a.pdf");
    assert(loaded_a != nullptr);
    assert(loaded_a->size == 18);
    std::cout << "   ✓ Index loaded and verified" << std::endl;

    // Show CSV content
    std::cout << "\n6. Index file content:" << std::endl;
    std::ifstream csv("test_index.csv");
    std::string line;
    while (std::getline(csv, line)) {
      std::cout << "   " << line << std::endl;
    }
    csv.close();

    std::cout << "\n✓ All tests passed!" << std::endl;
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
}
