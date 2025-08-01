# CMakeLists.txt

# Minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(LogAnalyzer CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. First, create the executable target from the source files
#    (Assumes LogParser.cpp is in 'src')
add_executable(log_analyzer main.cpp src/LogParser.cpp)

# 2. Now, modify the properties of the 'log_analyzer' target
#    Tell it where to find header files (Analytics.h, LogParser.h)
target_include_directories(log_analyzer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 3. Link the necessary threading libraries to the target
find_package(Threads REQUIRED)
target_link_libraries(log_analyzer PRIVATE Threads::Threads)