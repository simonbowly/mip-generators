
// needs seed variation, variation in entropy

#include <random>
#include <benchmark/benchmark.h>
#include "mipfeatures.hpp"

#define TESTCASE "tests/inst_566700647.mps"

using std::vector;
using namespace mipfeatures;


static void bm_integer_violations(benchmark::State& state) {
    const auto instance = MIPInstance::readMPS(TESTCASE);
    for (auto _ : state) {
        calculateIntegerViolations(instance);
    }
}
BENCHMARK(bm_integer_violations)->Unit(benchmark::kMicrosecond);


static void bm_sample_roundings(benchmark::State& state) {
    std::default_random_engine rng;
    const auto instance = MIPInstance::readMPS(TESTCASE);
    for (auto _ : state) {
        sampleRoundings(instance, 1000, rng);
    }
}
BENCHMARK(bm_sample_roundings)->Unit(benchmark::kMicrosecond);


static void bm_svd_range(benchmark::State& state) {
    const auto instance = MIPInstance::readMPS(TESTCASE);
    for (auto _ : state) {
        calculateSvdRange(instance);
    }
}
BENCHMARK(bm_svd_range)->Unit(benchmark::kMicrosecond);


static void bm_vc_graph(benchmark::State& state) {
    const auto instance = MIPInstance::readMPS(TESTCASE);
    for (auto _ : state) {
        vcGraphFeatures(instance);
    }
}
BENCHMARK(bm_vc_graph)->Unit(benchmark::kMicrosecond);


static void bm_bounds_slow(benchmark::State& state) {
    const auto instance = MIPInstance::readMPS(TESTCASE);
    for (auto _ : state) {
        getLpPolyhedralBounds(instance);
    }
}
BENCHMARK(bm_bounds_slow)->Unit(benchmark::kMicrosecond);


static void bm_bounds_fast(benchmark::State& state) {
    const auto instance = MIPInstance::readMPS(TESTCASE);
    for (auto _ : state) {
        getLpPolyhedralBoundsFast(instance);
    }
}
BENCHMARK(bm_bounds_fast)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();
