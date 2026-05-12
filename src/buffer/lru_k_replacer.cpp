#include "onebase/buffer/lru_k_replacer.h"
#include "onebase/common/exception.h"

namespace onebase {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k)
    : max_frames_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> guard(latch_);

  bool found_infinite = false;
  frame_id_t infinite_victim = INVALID_FRAME_ID;
  size_t infinite_earliest_ts = 0;

  bool found_finite = false;
  frame_id_t finite_victim = INVALID_FRAME_ID;
  size_t max_k_distance = 0;

  for (auto &[fid, entry] : entries_) {
    if (!entry.is_evictable_) {
      continue;
    }
    if (entry.history_.empty()) {
      continue;
    }

    if (entry.history_.size() < k_) {
      size_t first_access_ts = entry.history_.front();
      if (!found_infinite || first_access_ts < infinite_earliest_ts) {
        found_infinite = true;
        infinite_victim = fid;
        infinite_earliest_ts = first_access_ts;
      }
      continue;
    }

    size_t k_distance = current_timestamp_ - entry.history_.front();
    if (!found_finite || k_distance > max_k_distance) {
      found_finite = true;
      finite_victim = fid;
      max_k_distance = k_distance;
    }
  }

  if (!found_infinite && !found_finite) {
    return false;
  }

  frame_id_t victim = found_infinite ? infinite_victim : finite_victim;
  entries_.erase(victim);
  curr_size_--;
  *frame_id = victim;
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  std::lock_guard<std::mutex> guard(latch_);
  if (frame_id < 0 || static_cast<size_t>(frame_id) >= max_frames_) {
    throw OneBaseException("frame_id out of range", ExceptionType::OUT_OF_RANGE);
  }
  auto &entry = entries_[frame_id];
  entry.history_.push_back(current_timestamp_);
  if (entry.history_.size() > k_) {
    entry.history_.pop_front();
  }
  current_timestamp_++;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> guard(latch_);
  auto iter = entries_.find(frame_id);
  if (iter == entries_.end()) {
    return;
  }
  auto &entry = iter->second;
  if (entry.is_evictable_ != set_evictable) {
    if (set_evictable) {
      curr_size_++;
    } else {
      curr_size_--;
    }
    entry.is_evictable_ = set_evictable;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> guard(latch_);
  auto iter = entries_.find(frame_id);
  if (iter == entries_.end()) {
    return;
  }
  if (!iter->second.is_evictable_) {
    throw OneBaseException("cannot remove non-evictable frame", ExceptionType::INVALID);
  }
  entries_.erase(iter);
  curr_size_--;
}

auto LRUKReplacer::Size() const -> size_t {
  return curr_size_;
}

}  // namespace onebase
