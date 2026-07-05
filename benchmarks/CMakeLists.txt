add_executable(apex_bench bench_order_book.cpp)
target_link_libraries(apex_bench PRIVATE apex_lib benchmark::benchmark_main)
target_compile_options(apex_bench PRIVATE -O2 -DNDEBUG)