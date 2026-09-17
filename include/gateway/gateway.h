#pragma once

#include <memory>
#include <string>
#include <map>
#include "index/index.h"
#include "retrieval/retriever.h"

namespace openshard {
namespace gateway {

struct DatasetConfig {
  std::string name;
  std::string archive_dir;
  std::shared_ptr<index::Index> index;
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
  std::string HandleFileRequest(const std::string& dataset, const std::string& file);
  std::string HandleBatchRequest(const std::string& dataset, const std::string& body);
};

}  // namespace gateway
}  // namespace openshard
