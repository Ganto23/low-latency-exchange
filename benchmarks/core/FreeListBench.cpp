#include <benchmark/benchmark.h>
#include <pthread.h>
#include <sched.h>
#include <vector>

// Make sure this points to wherever you saved your FreeList code
#include "core/FreeList.hpp" 

inline void PinToIsolatedCore(int thread_idx) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    int core_id = (thread_idx % 3) + 1; 
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Dummy 32-byte struct to force the OS heap to do actual work
struct DummyOrder {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    uint32_t next_index;
    char padding[12]; 
};

// --- Benchmark 1: The OS Heap (new / delete) ---
static void BM_Standard_Heap_Allocation(benchmark::State& state) {
    PinToIsolatedCore(state.thread_index());
    for (auto _ : state) {
        // Force the OS to find memory on the heap
        DummyOrder* order = new DummyOrder();
        benchmark::DoNotOptimize(order);
        
        // Force the OS to reclaim it
        delete order;
    }
}
BENCHMARK(BM_Standard_Heap_Allocation)->Threads(1);

// --- Benchmark 2: Your Actual Index-Based FreeList ---
static void BM_Index_FreeList_Allocation(benchmark::State& state) {
    PinToIsolatedCore(state.thread_index());
    
    // Initialize your exact FreeList with 1 million capacity
    static FreeList<1000000> free_list;
    
    for (auto _ : state) {
        // Grab a uint32_t index (O(1) vector pop)
        uint32_t idx = free_list.allocate(); 
        benchmark::DoNotOptimize(idx);
        
        // Return the index (O(1) vector push)
        free_list.deallocate(idx); 
    }
}
BENCHMARK(BM_Index_FreeList_Allocation)->Threads(1);

BENCHMARK_MAIN();