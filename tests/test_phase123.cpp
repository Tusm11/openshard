#include "archive/tar_reader.h"
#include "index/index.h"
#include "retrieval/retriever.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <cstring>

using namespace openshard;

void CreateTestTar(const std::string& path) {
  std::ofstream tar(path, std::ios::binary);
  uint8_t header[512] = {0};
  std::strcpy((char*)header, "test.txt");
  std::strcpy((char*)header + 124, "00000000013");
  
  tar.write((char*)header, 512);
  tar.write("Hello World", 11);
  
  uint8_t padding[512] = {0};
  tar.write((char*)padding, 501);
  tar.write((char*)padding, 512);
  tar.write((char*)padding, 512);
  tar.close();
}

int main() {
  try {
    std::cout << "Testing Phase 1: TAR Reader..." << std::endl;
    CreateTestTar("test.tar");
    
    archive::TarReader reader("test.tar");
    if (!reader.Index()) {
      std::cerr << "Index failed" << std::endl;
      return 1;
    }
    if (reader.Files().size() == 0) {
      std::cerr << "No files in TAR" << std::endl;
      return 1;
    }
    const auto* file_entry = reader.GetFile("test.txt");
    if (!file_entry) {
      std::cerr << "File not found" << std::endl;
      return 1;
    }
    std::cout << "✓ Phase 1 passed" << std::endl;

    std::cout << "\nTesting Phase 2: Index..." << std::endl;
    auto idx = std::make_shared<index::Index>();
    idx->AddFile("test.txt", "test.tar", file_entry->offset, file_entry->size);
    
    auto csv = idx->ToJson();
    std::cout << "CSV output: " << csv << std::endl;
    
    idx->SaveToFile("test_index.csv");
    std::cout << "✓ Phase 2 passed" << std::endl;

    std::cout << "\nTesting Phase 3: Retriever..." << std::endl;
    retrieval::SelectiveRetriever retriever(idx);
    auto [data, metrics] = retriever.RetrieveFile("test.txt");
    std::cout << "✓ Phase 3 passed" << std::endl;

    std::cout << "\n✓ All tests passed!" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
}
