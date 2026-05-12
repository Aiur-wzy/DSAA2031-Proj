#include <gtest/gtest.h>
#include "onebase/buffer/lru_k_replacer.h"
#include "onebase/common/exception.h"

namespace onebase {

TEST(LRUKReplacerTest, BasicEviction) {
  // Basic smoke test after implementation: API calls should not throw.
  LRUKReplacer replacer(7, 2);

  frame_id_t frame;
  EXPECT_NO_THROW(replacer.Evict(&frame));
  EXPECT_NO_THROW(replacer.RecordAccess(1));
  EXPECT_NO_THROW(replacer.SetEvictable(1, true));
  EXPECT_NO_THROW(replacer.Remove(1));
  EXPECT_NO_THROW(replacer.Size());
}

// Students: After implementing LRU-K, add more comprehensive tests here:
// - Test with k=2, verify backward k-distance ordering
// - Test eviction with mix of evictable/non-evictable frames
// - Test that frames with <k accesses are evicted before frames with k accesses
// - Test Remove() on non-evictable frame throws

}  // namespace onebase
