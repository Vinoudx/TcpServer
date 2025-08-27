cmake_minimum_required(VERSION 3.0)
project(TcpServer)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20")
include_directories(${PROJECT_SOURCE_DIR}/include)

# 设置默认的并行编译
if(CMAKE_BUILD_PARALLEL_LEVEL)
  set(CMAKE_BUILD_PARALLEL_LEVEL 8) # 设置默认并行级别
endif()

add_subdirectory(./src)
add_subdirectory(./test)