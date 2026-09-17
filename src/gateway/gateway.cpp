#include "gateway/gateway.h"
#include "storage/range_retriever.h"
#include "httplib.h"
#include <iostream>
#include <algorithm>

namespace openshard {
namespace gateway {

class HttpServer {
 public:
  HttpServer(int port) : port_(port) {}

  bool Listen() {
    return server_.listen("0.0.0.0", port_);
  }

  void Stop() {
    server_.stop();
  }

  httplib::Server& GetServer() {
    return server_;
  }

 private:
  int port_;
  httplib::Server server_;
};

HttpGateway::HttpGateway(int port) : port_(port), running_(false) {
  server_ = std::make_unique<HttpServer>(port);
  
  // Register /health endpoint
  server_->GetServer().Get("/health", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("{\"status\":\"ok\"}", "application/json");
    res.status = 200;
  });

  // Register /datasets/{dataset}/files/{file} endpoint
  server_->GetServer().Get(
      R"(/datasets/([^/]+)/files/(.+))",
      [this](const httplib::Request& req, httplib::Response& res) {
        std::string dataset = req.matches[1];
        std::string file = req.matches[2];

        FileResponse response = HandleFileRequest(dataset, file);
        
        if (!response.success) {
          res.set_content("{\"error\": \"" + response.error + "\"}", "application/json");
          res.status = 404;
        } else {
          std::string content_type = GetContentType(file);
          res.set_content(
              std::string(response.data.begin(), response.data.end()),
              content_type.c_str()
          );
          res.status = 200;
        }
      }
  );
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

FileResponse HttpGateway::HandleFileRequest(const std::string& dataset, 
                                           const std::string& file) {
  FileResponse response{false, {}, 0, ""};

  auto it = datasets_.find(dataset);
  if (it == datasets_.end()) {
    response.error = "Dataset not found";
    return response;
  }

  const DatasetConfig& config = it->second;
  auto* entry = config.index->GetFile(file);
  if (!entry) {
    response.error = "File not found";
    return response;
  }

  // Local retrieval path
  if (config.IsLocal()) {
    retrieval::SelectiveRetriever retriever(config.index);
    auto [data, metrics] = retriever.RetrieveFile(file);

    if (!metrics.success) {
      response.error = "Retrieval failed";
      return response;
    }

    response.success = true;
    response.data = data;
    response.latency_ms = metrics.latency_ms;
    return response;
  }

  // Remote retrieval path (provider-neutral via RangeRetriever)
  if (config.IsRemote()) {
    storage::RangeRetriever retriever(config.archive_url);
    auto result = retriever.GetRange(config.archive_object, entry->offset, entry->size);

    if (!result.success) {
      response.error = result.error;
      return response;
    }

    response.success = true;
    response.data.resize(result.bytes_received);
    // Note: In real implementation, we'd capture the actual bytes from RangeRetriever
    // For now, this is a stub showing the integration point
    response.latency_ms = result.latency.count();
    return response;
  }

  response.error = "No archive configured (local or remote)";
  return response;
}

std::string HttpGateway::GetContentType(const std::string& filename) const {
  // Simple MIME type detection based on extension
  if (filename.size() > 4) {
    std::string ext = filename.substr(filename.size() - 4);
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".txt") return "text/plain";
    if (ext == ".csv") return "text/csv";
    if (ext == ".json") return "application/json";
  }
  
  return "application/octet-stream";
}

}  // namespace gateway
}  // namespace openshard
