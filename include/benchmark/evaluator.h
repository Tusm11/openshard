#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include "retrieval/retriever.h"
#include "index/index.h"

namespace openshard {
namespace benchmark {

enum class AccessPattern {
  VERY_SPARSE,  // <0.01% of files
  SPARSE,       // 1-10 files
  MODERATE,     // 1-20% of archive
  DENSE,        // 20%+ of archive
};

struct EvaluationResult {
  AccessPattern pattern;
  uint64_t files_requested;
  uint64_t archive_size;
  uint64_t data_needed;
  uint64_t selective_transfer;
  uint64_t bulk_transfer;
  double transfer_amplification_selective;
  double transfer_amplification_bulk;
  uint64_t selective_latency_ms;
  uint64_t bulk_latency_ms;
  double efficiency_ratio;  // bulk_transfer / selective_transfer
};

class ArchiveEvaluator {
 public:
  ArchiveEvaluator(std::shared_ptr<index::Index> index, uint64_t archive_size);

  // Measure selective vs bulk access
  EvaluationResult Evaluate(const std::vector<std::string>& requested_files,
                           AccessPattern pattern);

  // Simulate workload
  std::vector<EvaluationResult> SimulateWorkload(size_t num_patterns);

  // Generate report
  std::string GenerateReport(const std::vector<EvaluationResult>& results);

 private:
  std::shared_ptr<index::Index> index_;
  uint64_t archive_size_;

  std::vector<std::string> GenerateAccessPattern(AccessPattern pattern);
};

}  // namespace benchmark
}  // namespace openshard
