cmake_minimum_required(VERSION 3.5)
project(BinanceMonitor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 设置 Debug 模式和调试选项
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")


# 添加 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    main.cpp
    Worker.cpp
    BinanceAPI.cpp
    FeishuNotifier.cpp
    BinanceAPIManager.cpp
    OIAnalyzer.cpp
)

add_executable(monitor ${SOURCES})

# 链接依赖库
find_package(CURL REQUIRED)
target_link_libraries(monitor ${CURL_LIBRARIES} pthread)

# 打印调试信息
message(STATUS "CURL include: ${CURL_INCLUDE_DIRS}")
message(STATUS "CURL libs: ${CURL_LIBRARIES}")
