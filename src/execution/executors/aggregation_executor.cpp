#include "onebase/execution/executors/aggregation_executor.h"

#include <unordered_map>

namespace onebase {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                          std::unique_ptr<AbstractExecutor> child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void AggregationExecutor::Init() {
  child_executor_->Init();
  result_tuples_.clear();
  cursor_ = 0;

  struct AggState {
    std::vector<Value> group_values;
    std::vector<Value> aggregate_values;
    std::vector<bool> initialized;
  };

  std::unordered_map<std::string, AggState> groups;
  const auto &group_bys = plan_->GetGroupBys();
  const auto &aggregates = plan_->GetAggregates();
  const auto &agg_types = plan_->GetAggregateTypes();
  const auto &child_schema = child_executor_->GetOutputSchema();

  Tuple child_tuple;
  RID child_rid;
  while (child_executor_->Next(&child_tuple, &child_rid)) {
    std::vector<Value> group_values;
    group_values.reserve(group_bys.size());
    std::string group_key;
    for (const auto &expr : group_bys) {
      auto value = expr->Evaluate(&child_tuple, &child_schema);
      group_values.push_back(value);
      group_key += value.ToString();
      group_key += "|";
    }

    if (groups.find(group_key) == groups.end()) {
      AggState state;
      state.group_values = group_values;
      state.aggregate_values.reserve(aggregates.size());
      state.initialized.assign(aggregates.size(), false);
      for (size_t i = 0; i < aggregates.size(); i++) {
        switch (agg_types[i]) {
          case AggregationType::CountStarAggregate:
          case AggregationType::CountAggregate:
            state.aggregate_values.emplace_back(TypeId::INTEGER, 0);
            state.initialized[i] = true;
            break;
          case AggregationType::SumAggregate:
          case AggregationType::MinAggregate:
          case AggregationType::MaxAggregate:
            state.aggregate_values.emplace_back(aggregates[i]->GetReturnType());
            break;
        }
      }
      groups[group_key] = std::move(state);
    }

    auto &state = groups[group_key];
    for (size_t i = 0; i < aggregates.size(); i++) {
      auto input = aggregates[i]->Evaluate(&child_tuple, &child_schema);
      switch (agg_types[i]) {
        case AggregationType::CountStarAggregate:
          state.aggregate_values[i] = state.aggregate_values[i].Add(Value(TypeId::INTEGER, 1));
          break;
        case AggregationType::CountAggregate:
          if (!input.IsNull()) {
            state.aggregate_values[i] = state.aggregate_values[i].Add(Value(TypeId::INTEGER, 1));
          }
          break;
        case AggregationType::SumAggregate:
          if (!input.IsNull()) {
            if (!state.initialized[i]) {
              state.aggregate_values[i] = input;
              state.initialized[i] = true;
            } else {
              state.aggregate_values[i] = state.aggregate_values[i].Add(input);
            }
          }
          break;
        case AggregationType::MinAggregate:
          if (!input.IsNull()) {
            if (!state.initialized[i] || input.CompareLessThan(state.aggregate_values[i]).GetAsBoolean()) {
              state.aggregate_values[i] = input;
              state.initialized[i] = true;
            }
          }
          break;
        case AggregationType::MaxAggregate:
          if (!input.IsNull()) {
            if (!state.initialized[i] || input.CompareGreaterThan(state.aggregate_values[i]).GetAsBoolean()) {
              state.aggregate_values[i] = input;
              state.initialized[i] = true;
            }
          }
          break;
      }
    }
  }

  if (groups.empty() && group_bys.empty()) {
    std::vector<Value> row;
    row.reserve(aggregates.size());
    for (size_t i = 0; i < aggregates.size(); i++) {
      switch (agg_types[i]) {
        case AggregationType::CountStarAggregate:
        case AggregationType::CountAggregate:
          row.emplace_back(TypeId::INTEGER, 0);
          break;
        case AggregationType::SumAggregate:
        case AggregationType::MinAggregate:
        case AggregationType::MaxAggregate:
          row.emplace_back(aggregates[i]->GetReturnType());
          break;
      }
    }
    result_tuples_.emplace_back(std::move(row));
    return;
  }

  for (auto &[key, state] : groups) {
    std::vector<Value> row;
    row.reserve(state.group_values.size() + state.aggregate_values.size());
    for (auto &value : state.group_values) {
      row.push_back(value);
    }
    for (auto &value : state.aggregate_values) {
      row.push_back(value);
    }
    result_tuples_.emplace_back(std::move(row));
  }
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (cursor_ >= result_tuples_.size()) {
    return false;
  }
  *tuple = result_tuples_[cursor_++];
  *rid = tuple->GetRID();
  return true;
}

}  // namespace onebase
