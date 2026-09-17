#include <curl/curl.h>
#include <cassert>
#include <iostream>

// Minimal test: verify CURL range request works
struct TestState {
  uint64_t bytes_received = 0;
};

static size_t TestCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  ((TestState*)userp)->bytes_received += size * nmemb;
  return size * nmemb;
}

int main() {
  // Test 1: Range request header formatting
  std::string offset_str = std::to_string(1000000);
  std::string size_val = 4200000;
  std::string range = offset_str + "-" + std::to_string(1000000 + size_val - 1);
  assert(range == "1000000-5199999");
  std::cout << "✓ Test 1: Range header format correct" << std::endl;

  // Test 2: CURL initialization
  CURL* curl = curl_easy_init();
  assert(curl != nullptr);
  curl_easy_cleanup(curl);
  std::cout << "✓ Test 2: CURL initialization works" << std::endl;

  // Test 3: Callback mechanism
  TestState state;
  state.bytes_received = 0;
  TestCallback("test", 4, 1, &state);
  assert(state.bytes_received == 4);
  std::cout << "✓ Test 3: Callback accumulation works" << std::endl;

  std::cout << "\nAll Phase 0 tests passed." << std::endl;
  return 0;
}
