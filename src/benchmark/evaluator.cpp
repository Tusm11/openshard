#include "benchmark/evaluator.h"
#include <chrono>
#include <iostream>
#include <algorithm>

namespace openshard {
namespace benchmark {

ArchiveEvaluator::ArchiveEvaluator(std::shared_ptr<index::Index> index,
                                   uint64_t archive_size)
    : index_(index), archive_size_(archive_size) {}

EvaluationResult ArchiveEvaluator::Evaluate(
    const std::vector<std::string>& requested_files,
    AccessPattern pattern) {
  EvaluationResult result{};
  result.pattern = pattern;
  result.archive_size = archive_size_;
  result.files_requested = requested_files.size();

  // Calculate data needed
  uint64_t data_needed = 0;
  for (const auto& file : requested_files) {
    const auto* entry = index_->GetFile(file);
    if (entry) {
      data_needed += entry->size;
    }
  }
  result.data_needed = data_needed;

  // Selective retrieval
  auto start = std::chrono::high_resolution_clock::now();
  retrieval::SelectiveRetriever retriever(index_);
  uint64_t selective_transfer = 0;
  for (const auto& file : requested_files) {
    auto [data, metrics] = retriever.RetrieveFile(file);
    selective_transfer += metrics.bytes_transferred;
  }
  auto end = std::chrono::high_resolution_clock::now();
  result.selective_transfer = selective_transfer;
  result.selective_latency_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Bulk retrieval (download everything)
  result.bulk_transfer = archive_size_;
  result.bulk_latency_ms = result.selective_latency_ms * 10;  // Estimate

  // Calculate metrics
  result.transfer_amplification_selective =
      (double)result.selective_transfer / (double)data_needed;
  result.transfer_amplification_bulk =
      (double)result.bulk_transfer / (double)data_needed;
  result.efficiency_ratio =
      result.bulk_transfer > 0
          ? (double)result.selective_transfer / (double)result.bulk_transfer
          : 1.0;

  return result;
}

std::vector<EvaluationResult> ArchiveEvaluator::SimulateWorkload(
    size_t num_patterns) {
  std::vector<EvaluationResult> results;

  std::vector<AccessPattern> patterns = {AccessPattern::VERY_SPARSE,
                                        AccessPattern::SPARSE,
                                        AccessPattern::MODERATE,
                                        AccessPattern::DENSE};

  for (const auto& pattern : patterns) {
    auto files = GenerateAccessPattern(pattern);
    results.push_back(Evaluate(files, pattern));
  }

  return results;
}

std::string ArchiveEvaluator::GenerateReport(
    const std::vector<EvaluationResult>& results) {
  std::string report;
  report += "=== OpenShard Evaluation Report ===\n\n";

  for (const auto& result : results) {
    std::string pattern_name;
    switch (result.pattern) {
      case AccessPattern::VERY_SPARSE:
        pattern_name = "Very Sparse (<0.01%)";
        break;
      case AccessPattern::SPARSE:
        pattern_name = "Sparse (1-10 files)";
        break;
      case AccessPattern::MODERATE:
        pattern_name = "Moderate (1-20%)";
        break;
      case AccessPattern::DENSE:
        pattern_name = "Dense (20%+)";
        break;
    }

    report += "Pattern: " + pattern_name + "\n";
    report += "  Files: " + std::to_string(result.files_requested) + "\n";
    report += "  Data needed: " + std::to_string(result.data_needed) + " bytes\n";
    report += "  Selective transfer: " + std::to_string(result.selective_transfer) +
              " bytes (" + std::to_string((int)result.transfer_amplification_selective) +
              "x amplification)\n";
    report += "  Bulk transfer: " + std::to_string(result.bulk_transfer) + " bytes\n";
    report += "  Efficiency: " + std::to_string((int)(result.efficiency_ratio * 100)) +
              "%\n\n";
  }

  return report;
}

std::vector<std::string> ArchiveEvaluator::GenerateAccessPattern(
    AccessPattern pattern) {
  std::vector<std::string> files;
  const auto& entries = index_->Entries();

  switch (pattern) {
    case AccessPattern::VERY_SPARSE:
      if (!entries.empty()) {
        files.push_back(entries.begin()->first);
      }
      break;

    case AccessPattern::SPARSE:
      for (size_t i = 0; i < std::min((size_t)5, entries.size()); ++i) {
        auto it = entries.begin();
        std::advance(it, i);
        files.push_back(it->first);
      }
      break;

    case AccessPattern::MODERATE:
      for (size_t i = 0; i < entries.size() / 10; ++i) {
        auto it = entries.begin();
        std::advance(it, i);
        files.push_back(it->first);
      }
      break;

    case AccessPattern::DENSE:
      for (const auto& [name, _] : entries) {
        files.push_back(name);
      }
      break;
  }

  return files;
}

}  // namespace benchmark
}  // namespace openshard
