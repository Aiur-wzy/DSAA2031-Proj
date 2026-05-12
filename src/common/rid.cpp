#include "onebase/common/rid.h"

#include <string>

namespace onebase {

auto RID::ToString() const -> std::string {
    return "(" + std::to_string(page_id_) + ", " + std::to_string(slot_num_) + ")";
}

}  // namespace onebase
