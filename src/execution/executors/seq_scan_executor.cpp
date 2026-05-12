#include "onebase/execution/executors/seq_scan_executor.h"

namespace onebase {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void SeqScanExecutor::Init() {
  auto *catalog = GetExecutorContext()->GetCatalog();
  table_info_ = catalog->GetTable(plan_->GetTableOid());
  iter_ = table_info_->table_->Begin();
  end_ = table_info_->table_->End();
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (iter_ != end_) {
    auto current = *iter_;
    ++iter_;

    if (plan_->GetPredicate() != nullptr) {
      auto pred = plan_->GetPredicate()->Evaluate(&current, &GetOutputSchema());
      if (!pred.GetAsBoolean()) {
        continue;
      }
    }

    *tuple = current;
    *rid = current.GetRID();
    return true;
  }
  return false;
}

}  // namespace onebase
