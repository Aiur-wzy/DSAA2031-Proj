#include "onebase/execution/executors/index_scan_executor.h"

#include <unordered_map>

namespace onebase {

namespace {
struct IndexScanState {
  TableInfo *table_info{nullptr};
  TableHeap::Iterator iter{nullptr, RID(INVALID_PAGE_ID, 0)};
  TableHeap::Iterator end{nullptr, RID(INVALID_PAGE_ID, 0)};
};

std::unordered_map<const IndexScanExecutor *, IndexScanState> g_index_scan_state;
}  // namespace

IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::Init() {
  auto *catalog = GetExecutorContext()->GetCatalog();
  auto &state = g_index_scan_state[this];
  state.table_info = catalog->GetTable(plan_->GetTableOid());
  state.iter = state.table_info->table_->Begin();
  state.end = state.table_info->table_->End();
}

auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  auto it = g_index_scan_state.find(this);
  if (it == g_index_scan_state.end() || it->second.iter == it->second.end) {
    return false;
  }

  *tuple = *it->second.iter;
  *rid = tuple->GetRID();
  ++it->second.iter;
  return true;
}

}  // namespace onebase
