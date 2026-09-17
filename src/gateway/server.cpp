#include "gateway/gateway.h"
#include "index/index.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
  try {
    // Create gateway
    openshard::gateway::HttpGateway gateway(8080);

    // Register a test dataset
    auto index = std::make_shared<openshard::index::Index>();
    
    // Add test file to index
    // The retriever will read from "test_doc.pdf" at offset 0 for its entire size
    // We need to know the file size first
    std::ifstream file("test_doc.pdf", std::ios::binary | std::ios::ate);
    std::streamsize file_size = file.tellg();
    file.close();
    
    // Add file to index (archive name matches actual file)
    index->AddFile("test_doc.pdf", "test_doc.pdf", 0, file_size);

    openshard::gateway::DatasetConfig config;
    config.name = "legal";
    config.archive_dir = "./";
    config.index = index;

    gateway.RegisterDataset(config);

    std::cout << "OpenShard Server starting on port 8080..." << std::endl;
    std::cout << "Serving test_doc.pdf (" << file_size << " bytes)" << std::endl;
    std::cout << "Try:" << std::endl;
    std::cout << "  curl http://localhost:8080/health" << std::endl;
    std::cout << "  curl http://localhost:8080/datasets/legal/files/test_doc.pdf --output downloaded.pdf" << std::endl;

    gateway.Start();

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
