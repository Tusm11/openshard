#include <curl/curl.h>
#include <iostream>
#include <chrono>
#include <cstring>

// Callback to measure bytes received
struct DownloadState {
  uint64_t bytes_received = 0;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  ((DownloadState*)userp)->bytes_received += size * nmemb;
  return size * nmemb;
}

// Phase 0: Retrieve a byte range from a remote HTTP object
// Measures: bytes transferred, latency
int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: phase0_remote_range <url> <offset> <size>" << std::endl;
    std::cerr << "Example: phase0_remote_range http://example.com/file 1000000 4200000" << std::endl;
    return 1;
  }

  std::string url = argv[1];
  uint64_t offset = std::stoull(argv[2]);
  uint64_t size = std::stoull(argv[3]);

  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to initialize CURL" << std::endl;
    return 1;
  }

  DownloadState state;
  std::string range_header = "Range: bytes=" + std::to_string(offset) + 
                             "-" + std::to_string(offset + size - 1);

  auto start = std::chrono::high_resolution_clock::now();

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_RANGE, (std::to_string(offset) + "-" + 
                                         std::to_string(offset + size - 1)).c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = curl_easy_perform(curl);

  auto end = std::chrono::high_resolution_clock::now();
  auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    curl_easy_cleanup(curl);
    return 1;
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  // Output benchmark results
  std::cout << "Object URL:       " << url << std::endl;
  std::cout << "Requested offset: " << offset << std::endl;
  std::cout << "Requested size:   " << size << std::endl;
  std::cout << "Bytes received:   " << state.bytes_received << std::endl;
  std::cout << "Latency:          " << latency.count() << " ms" << std::endl;
  std::cout << "HTTP code:        " << http_code << std::endl;

  if (state.bytes_received == size || http_code == 206) {
    std::cout << "✓ Range retrieval successful" << std::endl;
  } else {
    std::cout << "✗ Range retrieval did not match expected size" << std::endl;
  }

  curl_easy_cleanup(curl);
  return 0;
}
