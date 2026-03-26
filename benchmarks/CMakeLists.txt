# Use CMake's built-in package manager to pull Google Benchmark
include(FetchContent)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
FetchContent_MakeAvailable(benchmark)

# Create the benchmark executable
add_executable(bitboard_bench core/BitboardBench.cpp)

# Point it to your source code headers
target_include_directories(bitboard_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link the Google Benchmark library and the pthread library
target_link_libraries(bitboard_bench PRIVATE benchmark::benchmark pthread)


# Create the SPSC Queue benchmark executable
add_executable(queue_bench concurrency/SPSCQueueBench.cpp)
target_include_directories(queue_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(queue_bench PRIVATE benchmark::benchmark pthread)

# Create the FreeList benchmark executable
add_executable(freelist_bench core/FreeListBench.cpp)
target_include_directories(freelist_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(freelist_bench PRIVATE benchmark::benchmark pthread)

# Create the Internal Pipeline benchmark
add_executable(pipeline_bench system/InternalPipelineBench.cpp)
target_include_directories(pipeline_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(pipeline_bench PRIVATE benchmark::benchmark pthread)

add_executable(stress_bench system/InternalPipelineStressBench.cpp)
target_include_directories(stress_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(stress_bench PRIVATE benchmark::benchmark pthread)