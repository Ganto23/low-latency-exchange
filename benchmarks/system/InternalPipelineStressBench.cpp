#include <benchmark/benchmark.h>
#include <pthread.h>
#include <vector>
#include <random>
#include "concurrency/SPSCQueue.hpp"
#include "core/Matcher.hpp"
#include "core/Order.hpp"

static SPSCQueue<OrderPayload, 65536> ingress_queue;
static SPSCQueue<ExecutionPayload, 1024> dummy_egress;
static SPSCQueue<MarketDataEvent, 4096> dummy_md;
static Matcher matcher(dummy_egress, dummy_md);
static std::vector<OrderPayload> chaos_script;

inline void PinToIsolatedCore(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Pre-compute the chaos so the PRNG overhead doesn't pollute timings
static void GenerateChaos() {
    if (!chaos_script.empty()) return;
    chaos_script.resize(1000000);
    
    // Track the side of every order we create so Cancels are accurate
    std::vector<bool> order_sides(1000000, false); 
    
    std::mt19937 gen(42); 
    std::uniform_int_distribution<uint32_t> price_dist(990, 1010); 
    std::uniform_int_distribution<uint32_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> buy_dist(0, 1);
    std::uniform_int_distribution<int> action_dist(1, 10);

    for (uint64_t i = 0; i < 1000000; i++) {
        // 20% chance to be a Cancel request for a recent order
        bool is_cancel = (i > 100) && (action_dist(gen) <= 2); 
        uint64_t target_id = is_cancel ? (i - (gen() % 100 + 1)) : i;
        
        bool side;
        if (is_cancel) {
            side = order_sides[target_id]; // Fetch the REAL side
        } else {
            side = buy_dist(gen) == 1;     // Generate a NEW side
            order_sides[i] = side;         // Save it
        }
        
        chaos_script[i] = {
            target_id,
            price_dist(gen),
            qty_dist(gen),
            side,
            is_cancel ? ActionType::Cancel : ActionType::New,
            0
        };
    }
}

static void BM_InternalPipeline_Chaos(benchmark::State& state) {
    if (state.thread_index() == 0) {
        // --- PRODUCER (The Firehose) ---
        GenerateChaos();
        PinToIsolatedCore(1);
        
        size_t idx = 0;
        for (auto _ : state) {
            // Push pre-calculated chaos instantly
            while (!ingress_queue.try_push(chaos_script[idx])) {
                asm volatile("yield" ::: "memory");
            }
            idx++;
        }
    } 
    else {
        // --- CONSUMER (The Matcher) ---
        PinToIsolatedCore(2);
        for (auto _ : state) {
            while (ingress_queue.available_to_read() == 0) {
                if (!state.KeepRunning()) break;
                asm volatile("yield" ::: "memory");
            }
            
            OrderPayload* p = ingress_queue.peek(0);
            
            // This is where your engine goes to war
            matcher.process_payload(*p); 

            ExecutionPayload temp_exec;
            while (dummy_egress.try_pop(temp_exec)) {
            }

            MarketDataEvent temp_md;
            while (dummy_md.try_pop(temp_md)) {
            }
            
            benchmark::DoNotOptimize(p);
            ingress_queue.advance(1);
        }
    }
}

// Hard-capped to 1,000,000 iterations to perfectly match the script 
// and prevent id_map wrapping/overflows.
BENCHMARK(BM_InternalPipeline_Chaos)->Threads(2)->Iterations(1000000);
BENCHMARK_MAIN();