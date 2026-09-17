#include "retrieval/planner.h"
#include <algorithm>
#include <iostream>

namespace openshard {
namespace retrieval {

RetrieverPlanner::RetrieverPlanner(std::shared_ptr<index::Index> index)
    : index_(index) {}

RetrievalPlan RetrieverPlanner::Plan(const std::vector<std::string>& filenames) {
  double clustering_ratio = CalculateClusteringRatio(filenames);

  // If clustering is efficient (>20% savings), use clustered ranges
  if (clustering_ratio > 0.8) {
    return PlanClustered(filenames);
  }

  // Otherwise use individual ranges
  return PlanIndividual(filenames);
}

double RetrieverPlanner::CalculateClusteringRatio(
    const std::vector<std::string>& filenames) const {
  std::vector<std::pair<uint64_t, uint64_t>> entries;

  for (const auto& filename : filenames) {
    const auto* entry = index_->GetFile(filename);
    if (entry) {
      entries.push_back({entry->offset, entry->size});
    }
  }

  if (entries.empty()) return 1.0;

  // Sort by offset
  std::sort(entries.begin(), entries.end());

  uint64_t individual_cost = 0;
  uint64_t clustered_cost = 0;
  uint64_t current_cluster_start = 0;
  uint64_t current_cluster_end = 0;

  for (const auto& [offset, size] : entries) {
    individual_cost += size;

    if (offset > current_cluster_end + CLUSTER_THRESHOLD) {
      clustered_cost += (current_cluster_end - current_cluster_start);
      current_cluster_start = offset;
    }
    current_cluster_end = offset + size;
  }

  clustered_cost += (current_cluster_end - current_cluster_start);

  if (individual_cost == 0) return 1.0;
  return (double)individual_cost / (double)clustered_cost;
}

uint64_t RetrieverPlanner::EstimateCost(const std::vector<std::string>& filenames,
                                        RetrievalStrategy strategy) const {
  uint64_t total = 0;

  for (const auto& filename : filenames) {
    const auto* entry = index_->GetFile(filename);
    if (entry) {
      total += entry->size;
    }
  }

  return total;
}

RetrievalPlan RetrieverPlanner::PlanIndividual(
    const std::vector<std::string>& filenames) const {
  RetrievalPlan plan;
  plan.strategy = RetrievalStrategy::INDIVIDUAL_RANGES;
  plan.num_requests = filenames.size();

  for (const auto& filename : filenames) {
    const auto* entry = index_->GetFile(filename);
    if (entry) {
      plan.ranges.push_back({entry->offset, entry->size});
      plan.estimated_transfer += entry->size;
    }
  }

  return plan;
}

RetrievalPlan RetrieverPlanner::PlanClustered(
    const std::vector<std::string>& filenames) const {
  RetrievalPlan plan;
  plan.strategy = RetrievalStrategy::CLUSTERED_RANGES;

  std::vector<std::pair<uint64_t, uint64_t>> entries;

  for (const auto& filename : filenames) {
    const auto* entry = index_->GetFile(filename);
    if (entry) {
      entries.push_back({entry->offset, entry->size});
    }
  }

  std::sort(entries.begin(), entries.end());

  uint64_t cluster_start = 0;
  uint64_t cluster_end = 0;

  for (const auto& [offset, size] : entries) {
    if (offset > cluster_end + CLUSTER_THRESHOLD) {
      plan.ranges.push_back({cluster_start, cluster_end - cluster_start});
      plan.num_requests++;
      cluster_start = offset;
    }
    cluster_end = offset + size;
  }

  if (cluster_end > cluster_start) {
    plan.ranges.push_back({cluster_start, cluster_end - cluster_start});
    plan.num_requests++;
  }

  plan.estimated_transfer = cluster_end - cluster_start;

  return plan;
}

RetrievalPlan RetrieverPlanner::PlanFullArchive(
    const std::vector<std::string>& filenames) const {
  RetrievalPlan plan;
  plan.strategy = RetrievalStrategy::FULL_ARCHIVE;
  plan.num_requests = 1;
  plan.ranges.push_back({0, UINT64_MAX});
  plan.estimated_transfer = UINT64_MAX;
  return plan;
}

}  // namespace retrieval
}  // namespace openshard
