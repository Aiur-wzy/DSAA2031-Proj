#include "onebase/storage/page/b_plus_tree_internal_page.h"
#include <functional>
#include "onebase/common/exception.h"

namespace onebase {

template class BPlusTreeInternalPage<int, page_id_t, std::less<int>>;

// ---------------------------------------------------------------------------
// Provided helpers (same as student stub)
// ---------------------------------------------------------------------------

template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetMaxSize(max_size);
  SetSize(0);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  return array_[index].first;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) {
  array_[index].first = key;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  return array_[index].second;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetValueAt(int index, const ValueType &value) {
  array_[index].second = value;
}

// ---------------------------------------------------------------------------
// Reference implementations
// ---------------------------------------------------------------------------


template <typename KeyType, typename ValueType, typename KeyComparator>
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value) const -> int {
  for (int i = 0; i < GetSize(); i++) {
    if (array_[i].second == value) {
      return i;
    }
  }
  return -1;
}


template <typename KeyType, typename ValueType, typename KeyComparator>
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::Lookup(const KeyType &key,
                                            const KeyComparator &comparator) const -> ValueType {
  // Scan from the last key down to index 1.  The first index i where
  // array_[i].first <= key (i.e. NOT (key < array_[i].first)) gives the
  // correct child pointer.
  for (int i = GetSize() - 1; i >= 1; i--) {
    if (!comparator(key, array_[i].first)) {
      // key >= array_[i].first
      return array_[i].second;
    }
  }
  // key < all keys => leftmost child
  return array_[0].second;
}


template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::PopulateNewRoot(const ValueType &old_value,
                                                      const KeyType &key,
                                                      const ValueType &new_value) {
  array_[0].second = old_value;
  array_[1].first = key;
  array_[1].second = new_value;
  SetSize(2);
}


template <typename KeyType, typename ValueType, typename KeyComparator>
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertNodeAfter(const ValueType &old_value,
                                                      const KeyType &key,
                                                      const ValueType &new_value) -> int {
  int idx = ValueIndex(old_value);
  // Shift entries [idx+1 .. size_) one position to the right.
  for (int i = GetSize(); i > idx + 1; i--) {
    array_[i] = array_[i - 1];
  }
  array_[idx + 1].first = key;
  array_[idx + 1].second = new_value;
  IncreaseSize(1);
  return GetSize();
}


template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Remove(int index) {
  for (int i = index; i < GetSize() - 1; i++) {
    array_[i] = array_[i + 1];
  }
  IncreaseSize(-1);
}

/**
 * Remove all entries and return the only remaining child pointer
 * (array_[0].second).  Sets size to 0.
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::RemoveAndReturnOnlyChild() -> ValueType {
  ValueType child = array_[0].second;
  SetSize(0);
  return child;
}


template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveAllTo(BPlusTreeInternalPage *recipient,
                                                const KeyType &middle_key) {
  int r_size = recipient->GetSize();

  // The key for entry [0] of this page is meaningless; use middle_key instead.
  recipient->array_[r_size].first = middle_key;
  recipient->array_[r_size].second = array_[0].second;

  for (int i = 1; i < GetSize(); i++) {
    recipient->array_[r_size + i] = array_[i];
  }

  recipient->IncreaseSize(GetSize());
  SetSize(0);
}


template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveHalfTo(BPlusTreeInternalPage *recipient,
                                                  const KeyType &middle_key) {
  int split = GetSize() / 2;

  // The entry at `split` has the key that will be pushed to the parent
  // (middle_key).  Its child becomes the leftmost child of recipient.
  // Entries [split+1 .. size_) are the remaining children with their keys.

  // recipient array_[0].second = our array_[split].second  (leftmost child)
  // recipient array_[0].first  = middle_key (unused by convention but set for clarity)
  recipient->array_[0].second = array_[split].second;

  int j = 1;
  for (int i = split + 1; i < GetSize(); i++, j++) {
    recipient->array_[j] = array_[i];
  }

  recipient->SetSize(j);
  SetSize(split);
}


template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveFirstToEndOf(BPlusTreeInternalPage *recipient,
                                                       const KeyType &middle_key) {
  int r_size = recipient->GetSize();

  // Append to recipient: middle_key + our leftmost child pointer.
  recipient->array_[r_size].first = middle_key;
  recipient->array_[r_size].second = array_[0].second;
  recipient->IncreaseSize(1);

  // Shift our entries [1 .. size_) left to [0 .. size_-1).
  for (int i = 0; i < GetSize() - 1; i++) {
    array_[i] = array_[i + 1];
  }
  IncreaseSize(-1);
}


template <typename KeyType, typename ValueType, typename KeyComparator>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveLastToFrontOf(BPlusTreeInternalPage *recipient,
                                                        const KeyType &middle_key) {
  int r_size = recipient->GetSize();

  // Shift recipient entries right by 1.
  for (int i = r_size; i > 0; i--) {
    recipient->array_[i] = recipient->array_[i - 1];
  }

  // Place our last entry at front of recipient.
  int last = GetSize() - 1;
  recipient->array_[0].second = array_[last].second;
  recipient->array_[1].first = middle_key;

  recipient->IncreaseSize(1);
  IncreaseSize(-1);
}

}  // namespace onebase
