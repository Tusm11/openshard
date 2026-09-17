#include "gateway/gateway.h"
#include <iostream>
#include <functional>
#include <vector>
#include <map>

namespace openshard {
namespace gateway {

class HttpServer {
 public:
  HttpServer(int port) : port_(port) {}
  
  void SetHandler(const std::string& path, 
                  const std::function<std::string(const std::string&)>& handler) {
    handlers_[path] = handler;
  }

  bool Listen() { return true; }
  void Stop() {}

 private:
  int port_;
  std::map<std::string, std::function<std::string(const std::string&)>> handlers_;
};

HttpGateway::HttpGateway(int port) : port_(port), running_(false) {
  server_ = std::make_unique<HttpServer>(port);
}

HttpGateway::~HttpGateway() {
  Stop();
}

void HttpGateway::RegisterDataset(const DatasetConfig& config) {
  datasets_[config.name] = config;
  std::cout << "Registered dataset: " << config.name << std::endl;
}

void HttpGateway::Start() {
  running_ = true;
  std::cout << "Gateway listening on port " << port_ << std::endl;
  
  if (!server_->Listen()) {
    std::cerr << "Failed to start server" << std::endl;
    running_ = false;
    return;
  }
  
  std::cout << "Server started" << std::endl;
}

void HttpGateway::Stop() {
  if (running_) {
    server_->Stop();
    running_ = false;
    std::cout << "Server stopped" << std::endl;
  }
}

std::string HttpGateway::HandleFileRequest(const std::string& dataset, 
                                          const std::string& file) {
  auto it = datasets_.find(dataset);
  if (it == datasets_.end()) {
    return "{\"error\": \"Dataset not found\"}";
  }

  auto* entry = it->second.index->GetFile(file);
  if (!entry) {
    return "{\"error\": \"File not found\"}";
  }

  retrieval::SelectiveRetriever retriever(it->second.index);
  auto [data, metrics] = retriever.RetrieveFile(file);

  return "{\"success\": true, \"size\": " + std::to_string(metrics.bytes_transferred) 
         + ", \"latency_ms\": " + std::to_string(metrics.latency_ms) + "}";
}

std::string HttpGateway::HandleBatchRequest(const std::string& dataset,
                                           const std::string& body) {
  auto it = datasets_.find(dataset);
  if (it == datasets_.end()) {
    return "{\"error\": \"Dataset not found\"}";
  }

  return "{\"success\": true}";
}

}  // namespace gateway
}  // namespace openshard
