#include <benchmark/benchmark.h>
#include "core/Bitboard.hpp"

// Benchmark setting a price level as active
static void BM_Bitboard_SetActive(benchmark::State& state) {
    Bitboard board;
    for (auto _ : state) {
        board.set_active(5000);
        // DoNotOptimize forces the compiler to actually execute the memory write
        benchmark::DoNotOptimize(board); 
    }
}
BENCHMARK(BM_Bitboard_SetActive);

// Benchmark setting a price level as empty
static void BM_Bitboard_SetEmpty(benchmark::State& state) {
    Bitboard board;
    board.set_active(5000);
    for (auto _ : state) {
        board.set_empty(5000);
        benchmark::DoNotOptimize(board);
    }
}
BENCHMARK(BM_Bitboard_SetEmpty);

// Benchmark the std::countr_zero hardware instruction
static void BM_Bitboard_FindLowestAsk(benchmark::State& state) {
    Bitboard board;
    board.set_active(5000); // Set a dummy ask
    board.set_active(5100);
    
    for (auto _ : state) {
        uint32_t lowest_ask = board.find_lowest_ask();
        // DoNotOptimize forces the CPU to actually calculate the result
        benchmark::DoNotOptimize(lowest_ask);
    }
}
BENCHMARK(BM_Bitboard_FindLowestAsk);

// Benchmark the std::countl_zero hardware instruction
static void BM_Bitboard_FindHighestBid(benchmark::State& state) {
    Bitboard board;
    board.set_active(4900); // Set a dummy bid
    board.set_active(4800);
    
    for (auto _ : state) {
        uint32_t highest_bid = board.find_highest_bid();
        benchmark::DoNotOptimize(highest_bid);
    }
}
BENCHMARK(BM_Bitboard_FindHighestBid);

// This macro generates the main() function for us
BENCHMARK_MAIN();