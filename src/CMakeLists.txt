# Create the executable from your main entry point
add_executable(matching_engine main.cpp)

# Allow absolute-style includes from the src directory
target_include_directories(matching_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the Linux pthread library for core pinning (pthread_setaffinity_np)
target_link_libraries(matching_engine PRIVATE pthread)