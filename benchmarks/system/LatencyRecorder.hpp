#ifndef LATENCY_RECORDER_HPP
#define LATENCY_RECORDER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <iomanip>
#include <string>

template<size_t MaxLatencyNs>
class LatencyRecorder {
public:
    uint64_t buckets[MaxLatencyNs + 1] = {0};
    uint64_t overflows = 0;
    uint64_t total_samples = 0;

    inline void __attribute__((always_inline)) record(uint64_t latency_ns) {
        if (latency_ns <= MaxLatencyNs) {
            buckets[latency_ns]++;
        } else {
            overflows++;
        }
        total_samples++;
    }

    void save_to_csv(const std::string& filename) {
        std::ofstream file(filename);
        file << "latency_ns,count\n";
        for (uint64_t ns = 0; ns <= MaxLatencyNs; ++ns) {
            if (buckets[ns] > 0) {
                file << ns << "," << buckets[ns] << "\n";
            }
        }
        file.close();
        std::cout << "[Telemetry] Raw latency data saved to " << filename << std::endl;
    }

    void report(const std::string& label) {
        if (total_samples == 0) return;

        uint64_t processed = 0;
        double percentiles[] = {0.50, 0.90, 0.99, 0.999, 0.9999};
        int p_idx = 0;

        std::cout << "\n==========================================" << std::endl;
        std::cout << " LATENCY REPORT: " << label << std::endl;
        std::cout << "==========================================" << std::endl;
        
        for (uint64_t ns = 0; ns <= MaxLatencyNs && p_idx < 5; ++ns) {
            processed += buckets[ns];
            while (p_idx < 5 && (double)processed / total_samples >= percentiles[p_idx]) {
                std::cout << std::left << std::setw(10) << ("P" + std::to_string(percentiles[p_idx] * 100)) 
                          << ": " << ns << " ns" << std::endl;
                p_idx++;
            }
        }
        
        if (overflows > 0) {
            std::cout << "Outliers (> " << MaxLatencyNs << "ns): " << overflows 
                      << " (" << std::fixed << std::setprecision(4) 
                      << (double)overflows/total_samples * 100 << "%)" << std::endl;
        }
        std::cout << "==========================================\n" << std::endl;
    }
};

#endif