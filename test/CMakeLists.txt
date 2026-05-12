# test/CMakeLists.txt

include(GoogleTest)

if(NOT GTest_FOUND)
  add_executable(onebase_smoke_test smoke/smoke_test_main.cpp)
  target_link_libraries(onebase_smoke_test PRIVATE onebase)
  target_compile_features(onebase_smoke_test PRIVATE cxx_std_17)
  add_test(NAME onebase_smoke_test COMMAND onebase_smoke_test)
  return()
endif()

# Convenience macro for adding tests
macro(onebase_add_test TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})
  target_link_libraries(${TEST_NAME} PRIVATE onebase onebase_storage GTest::gtest_main)
  target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
  gtest_discover_tests(${TEST_NAME})
endmacro()

# Type tests
onebase_add_test(value_test type/value_test.cpp)

# Buffer pool tests (Lab 1)
onebase_add_test(lru_k_replacer_test buffer/lru_k_replacer_test.cpp)
onebase_add_test(buffer_pool_manager_test buffer/buffer_pool_manager_test.cpp)
onebase_add_test(page_guard_test buffer/page_guard_test.cpp)

# Storage tests
onebase_add_test(disk_manager_test storage/disk_manager_test.cpp)

# B+ tree tests (Lab 2)
onebase_add_test(b_plus_tree_insert_test storage/b_plus_tree_insert_test.cpp)
onebase_add_test(b_plus_tree_delete_test storage/b_plus_tree_delete_test.cpp)

# Execution tests (Lab 3)
onebase_add_test(executor_test execution/executor_test.cpp)

# Concurrency tests (Lab 4)
onebase_add_test(lock_manager_test concurrency/lock_manager_test.cpp)

# ──────────────────────────────────────────────
# Evaluation (graded) tests — custom main()
# ──────────────────────────────────────────────
macro(onebase_eval_test TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${TEST_SOURCE} eval/eval_main.cpp)
  target_link_libraries(${TEST_NAME} PRIVATE onebase onebase_storage GTest::gtest)
  target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
  gtest_discover_tests(${TEST_NAME} TEST_PREFIX "${TEST_NAME}.")
endmacro()

onebase_eval_test(lab1_eval_test eval/lab1_eval_test.cpp)
onebase_eval_test(lab2_eval_test eval/lab2_eval_test.cpp)
onebase_eval_test(lab3_eval_test eval/lab3_eval_test.cpp)
onebase_eval_test(lab4_eval_test eval/lab4_eval_test.cpp)
