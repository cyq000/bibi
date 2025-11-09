cmake_minimum_required(VERSION 3.10)
project(trend_monitor CXX)
set(CMAKE_CXX_STANDARD 17)

# include header directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

file(GLOB SRC_FILES "src/*.cpp")
add_executable(trend_monitor ${SRC_FILES})