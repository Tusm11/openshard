#include "storage/range_retriever.h"
#ifdef HAVE_CURL
#include <curl/curl.h>
#endif
#include <chrono>
#include <iostream>

namespace openshard {
namespace storage {

#ifdef HAVE_CURL
struct CurlBuffer {
  std::vector<uint8_t> data;
  size_t bytes_received;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t realsize = size * nmemb;
  CurlBuffer* buf = (CurlBuffer*)userp;

  uint8_t* ptr = (uint8_t*)realloc(nullptr, buf->data.size() + realsize);
  if (!ptr) {
    std::cerr << "Not enough memory for curl callback" << std::endl;
    return 0;
  }

  if (!buf->data.empty()) {
    std::memcpy(ptr, buf->data.data(), buf->data.size());
  }
  std::memcpy(&ptr[buf->data.size()], contents, realsize);

  buf->data.assign(ptr, ptr + buf->data.size() + realsize);
  buf->bytes_received += realsize;
  free(ptr);

  return realsize;
}
#endif

RangeRetriever::RangeRetriever(const std::string& endpoint)
    : endpoint_(endpoint) {}

RangeRetriever::~RangeRetriever() = default;

RangeRetrievalResult RangeRetriever::GetRange(const std::string& object_key,
                                             uint64_t offset,
                                             uint64_t size) {
  RangeRetrievalResult result{offset, size, 0, std::chrono::milliseconds(0), false, ""};

#ifndef HAVE_CURL
  result.error = "CURL not available";
  return result;
#else
  CURL* curl = curl_easy_init();
  if (!curl) {
    result.error = "Failed to initialize CURL";
    return result;
  }

  // Construct full URL
  std::string url = endpoint_ + object_key;

  // Construct range header
  std::string range = std::to_string(offset) + "-" + std::to_string(offset + size - 1);

  CurlBuffer buf;
  auto start = std::chrono::high_resolution_clock::now();

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buf);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

  CURLcode res = curl_easy_perform(curl);

  auto end = std::chrono::high_resolution_clock::now();
  result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  if (res != CURLE_OK) {
    result.error = std::string(curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return result;
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  // Accept 200 (full file) or 206 (partial content)
  if (http_code == 200 || http_code == 206) {
    result.bytes_received = buf.bytes_received;
    result.success = (buf.bytes_received == size);
  } else {
    result.error = "HTTP " + std::to_string(http_code);
  }

  curl_easy_cleanup(curl);
  return result;
#endif
}

uint64_t RangeRetriever::GetObjectSize(const std::string& object_key) {
#ifndef HAVE_CURL
  return 0;
#else
  CURL* curl = curl_easy_init();
  if (!curl) {
    return 0;
  }

  std::string url = endpoint_ + object_key;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = curl_easy_perform(curl);

  uint64_t size = 0;
  if (res == CURLE_OK) {
    double content_length = 0;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
    if (content_length > 0) {
      size = (uint64_t)content_length;
    }
  }

  curl_easy_cleanup(curl);
  return size;
#endif
}

}  // namespace storage
}  // namespace openshard
