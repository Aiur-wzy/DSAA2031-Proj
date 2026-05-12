#include "onebase/buffer/buffer_pool_manager.h"
#include "onebase/common/exception.h"
#include "onebase/common/logger.h"

namespace onebase {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k)
    : pool_size_(pool_size), disk_manager_(disk_manager) {
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<frame_id_t>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> guard(latch_);

  frame_id_t frame_id;
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
  } else if (!replacer_->Evict(&frame_id)) {
    return nullptr;
  }

  auto &frame = pages_[frame_id];
  if (frame.GetPageId() != INVALID_PAGE_ID) {
    if (frame.IsDirty()) {
      disk_manager_->WritePage(frame.GetPageId(), frame.GetData());
    }
    page_table_.erase(frame.GetPageId());
  }

  *page_id = disk_manager_->AllocatePage();
  frame.ResetMemory();
  frame.page_id_ = *page_id;
  frame.pin_count_ = 1;
  frame.is_dirty_ = false;
  page_table_[*page_id] = frame_id;

  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  return &frame;
}

auto BufferPoolManager::FetchPage(page_id_t page_id) -> Page * {
  std::lock_guard<std::mutex> guard(latch_);

  auto found = page_table_.find(page_id);
  if (found != page_table_.end()) {
    auto frame_id = found->second;
    auto &frame = pages_[frame_id];
    frame.pin_count_++;
    replacer_->RecordAccess(frame_id);
    replacer_->SetEvictable(frame_id, false);
    return &frame;
  }

  frame_id_t frame_id;
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
  } else if (!replacer_->Evict(&frame_id)) {
    return nullptr;
  }

  auto &frame = pages_[frame_id];
  if (frame.GetPageId() != INVALID_PAGE_ID) {
    if (frame.IsDirty()) {
      disk_manager_->WritePage(frame.GetPageId(), frame.GetData());
    }
    page_table_.erase(frame.GetPageId());
  }

  disk_manager_->ReadPage(page_id, frame.GetData());
  frame.page_id_ = page_id;
  frame.pin_count_ = 1;
  frame.is_dirty_ = false;
  page_table_[page_id] = frame_id;

  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  return &frame;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
  std::lock_guard<std::mutex> guard(latch_);
  auto found = page_table_.find(page_id);
  if (found == page_table_.end()) {
    return false;
  }

  auto frame_id = found->second;
  auto &frame = pages_[frame_id];
  if (frame.pin_count_ <= 0) {
    return false;
  }

  if (is_dirty) {
    frame.is_dirty_ = true;
  }
  frame.pin_count_--;
  if (frame.pin_count_ == 0) {
    replacer_->SetEvictable(frame_id, true);
  }
  return true;
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> guard(latch_);
  auto found = page_table_.find(page_id);
  if (found == page_table_.end()) {
    disk_manager_->DeallocatePage(page_id);
    return true;
  }

  auto frame_id = found->second;
  auto &frame = pages_[frame_id];
  if (frame.pin_count_ > 0) {
    return false;
  }

  replacer_->Remove(frame_id);
  page_table_.erase(page_id);
  frame.ResetMemory();
  frame.page_id_ = INVALID_PAGE_ID;
  frame.pin_count_ = 0;
  frame.is_dirty_ = false;
  free_list_.push_back(frame_id);
  disk_manager_->DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> guard(latch_);
  auto found = page_table_.find(page_id);
  if (found == page_table_.end()) {
    return false;
  }
  auto &frame = pages_[found->second];
  disk_manager_->WritePage(page_id, frame.GetData());
  frame.is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::lock_guard<std::mutex> guard(latch_);
  for (const auto &[page_id, frame_id] : page_table_) {
    auto &frame = pages_[frame_id];
    disk_manager_->WritePage(page_id, frame.GetData());
    frame.is_dirty_ = false;
  }
}

}  // namespace onebase
