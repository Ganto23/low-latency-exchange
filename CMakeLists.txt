cmake_minimum_required(VERSION 3.15)
project(low-latency-exchange CXX)

# Enforce C++20 for the bitboard hardware instructions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bare-metal HFT compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -flto -Wall -Wextra")

# Tell CMake to look in the src folder
add_subdirectory(src)