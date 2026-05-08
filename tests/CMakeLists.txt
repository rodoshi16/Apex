enable_testing()

add_executable(apex_tests test_order.cpp)
target_link_libraries(apex_tests PRIVATE apex_lib GTest::gtest_main)
target_include_directories(apex_tests PRIVATE ${gtest_SOURCE_DIR}/include)

add_test(NAME apex_tests COMMAND apex_tests)