#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include "index/index.h"
#include "retrieval/retriever.h"

namespace openshard {
namespace gateway {

struct DatasetConfig {
  std::string name;
  std::string archive_dir;
  std::shared_ptr<index::Index> index;
};

struct FileResponse {
  bool success;
  std::vector<uint8_t> data;
  double latency_ms;
  std::string error;
};

class HttpGateway {
 public:
  HttpGateway(int port);
  ~HttpGateway();

  // Register dataset
  void RegisterDataset(const DatasetConfig& config);

  // Start server (blocking until stopped)
  void Start();

  // Stop server
  void Stop();

  // Check if running
  bool IsRunning() const { return running_; }

 private:
  int port_;
  bool running_;
  std::map<std::string, DatasetConfig> datasets_;
  std::unique_ptr<class HttpServer> server_;

  // Internal request handlers
  FileResponse HandleFileRequest(const std::string& dataset, const std::string& file);
  std::string GetContentType(const std::string& filename) const;
};

}  // namespace gateway
}  // namespace openshard
