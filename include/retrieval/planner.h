#pragma once

#include <vector>
#include <string>
#include <memory>
#include "index/index.h"

namespace openshard {
namespace retrieval {

enum class RetrievalStrategy {
  INDIVIDUAL_RANGES,  // Request each file separately
  CLUSTERED_RANGES,   // Group nearby files into single range
  FULL_ARCHIVE,       // Download entire archive (fallback)
};

struct RetrievalPlan {
  RetrievalStrategy strategy;
  std::vector<std::pair<uint64_t, uint64_t>> ranges;  // (offset, size) pairs
  uint64_t estimated_transfer;
  uint64_t num_requests;
};

class RetrieverPlanner {
 public:
  explicit RetrieverPlanner(std::shared_ptr<index::Index> index);

  // Plan retrieval for file set
  RetrievalPlan Plan(const std::vector<std::string>& filenames);

  // Analyze clustering efficiency
  double CalculateClusteringRatio(const std::vector<std::string>& filenames) const;

  // Estimate transfer cost per strategy
  uint64_t EstimateCost(const std::vector<std::string>& filenames,
                       RetrievalStrategy strategy) const;

 private:
  std::shared_ptr<index::Index> index_;
  static constexpr uint64_t CLUSTER_THRESHOLD = 1024 * 1024;  // 1 MB

  RetrievalPlan PlanIndividual(const std::vector<std::string>& filenames) const;
  RetrievalPlan PlanClustered(const std::vector<std::string>& filenames) const;
  RetrievalPlan PlanFullArchive(const std::vector<std::string>& filenames) const;
};

}  // namespace retrieval
}  // namespace openshard
