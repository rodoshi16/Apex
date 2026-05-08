#include <benchmark/benchmark.h>

static void BM_Placeholder(benchmark::State& state) {
    for (auto _ : state) {
        // benchmarks coming soon
    }
}
BENCHMARK(BM_Placeholder);

BENCHMARK_MAIN();