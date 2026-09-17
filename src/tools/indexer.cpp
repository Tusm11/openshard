#include "index/index_builder.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: openshard_indexer <tar_file> <output_index>" << std::endl;
    std::cerr << "Example: openshard_indexer data.tar data.index.csv" << std::endl;
    return 1;
  }

  std::string tar_path = argv[1];
  std::string index_path = argv[2];

  std::cout << "OpenShard Index Builder" << std::endl;
  std::cout << "======================" << std::endl;
  std::cout << "TAR file:     " << tar_path << std::endl;
  std::cout << "Output index: " << index_path << std::endl;
  std::cout << std::endl;

  openshard::index::IndexBuilder builder;

  std::cout << "Indexing TAR..." << std::endl;
  if (!builder.BuildAndSave(tar_path, index_path)) {
    std::cerr << "Failed to build index" << std::endl;
    return 1;
  }

  std::cout << std::endl;
  std::cout << "✓ Index complete" << std::endl;
  std::cout << "  Files indexed: " << builder.GetFileCount() << std::endl;
  std::cout << "  Output:        " << index_path << std::endl;

  return 0;
}
