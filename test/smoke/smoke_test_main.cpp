#include <cstring>
#include <filesystem>
#include <iostream>

#include "onebase/common/config.h"
#include "onebase/storage/disk/disk_manager.h"
#include "onebase/type/type.h"
#include "onebase/type/value.h"

namespace onebase {

namespace {

auto Check(bool cond, const std::string &name) -> bool {
  if (!cond) {
    std::cerr << "[FAIL] " << name << '\n';
    return false;
  }
  std::cout << "[PASS] " << name << '\n';
  return true;
}

auto RunValueSmokeTest() -> bool {
  bool ok = true;
  Value lhs(TypeId::INTEGER, 10);
  Value rhs(TypeId::INTEGER, 3);

  ok &= Check(lhs.Add(rhs).GetAsInteger() == 13, "Value.Add INTEGER");
  ok &= Check(lhs.Subtract(rhs).GetAsInteger() == 7, "Value.Subtract INTEGER");
  ok &= Check(lhs.Multiply(rhs).GetAsInteger() == 30, "Value.Multiply INTEGER");
  ok &= Check(lhs.Divide(rhs).GetAsInteger() == 3, "Value.Divide INTEGER");
  ok &= Check(lhs.CompareGreaterThan(rhs).GetAsBoolean(), "Value.CompareGreaterThan INTEGER");

  return ok;
}

auto RunDiskManagerSmokeTest() -> bool {
  bool ok = true;
  const auto db_path = (std::filesystem::temp_directory_path() / "onebase_smoke_disk.db").string();
  DiskManager dm(db_path);

  char write_buf[ONEBASE_PAGE_SIZE];
  char read_buf[ONEBASE_PAGE_SIZE];
  std::memset(write_buf, 0, ONEBASE_PAGE_SIZE);
  std::strcpy(write_buf, "onebase_smoke");

  const auto page_id = dm.AllocatePage();
  dm.WritePage(page_id, write_buf);
  dm.ReadPage(page_id, read_buf);

  ok &= Check(page_id == 0, "DiskManager.AllocatePage");
  ok &= Check(std::strcmp(write_buf, read_buf) == 0, "DiskManager.ReadWritePage");

  dm.ShutDown();
  std::filesystem::remove(db_path);
  return ok;
}

}  // namespace

}  // namespace onebase

auto main() -> int {
  bool ok = true;
  ok &= onebase::RunValueSmokeTest();
  ok &= onebase::RunDiskManagerSmokeTest();
  return ok ? 0 : 1;
}
