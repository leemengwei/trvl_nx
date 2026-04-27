cmake_minimum_required(VERSION 3.14)
project(trvl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# 基本 include 路径
# -----------------------------
include_directories("${PROJECT_SOURCE_DIR}/src")

# -----------------------------
# 查找依赖
# -----------------------------
find_package(pybind11 REQUIRED)
find_package(OpenCV REQUIRED)

# 自动查找 Brotli 库
find_path(BROTLI_INCLUDE_DIRS "brotli/encode.h" "brotli/decode.h")
find_library(BROTLIENC_LIB NAMES brotlienc)
find_library(BROTLIDEC_LIB NAMES brotlidec)

if(NOT BROTLIENC_LIB OR NOT BROTLIDEC_LIB)
    message(FATAL_ERROR "Cannot find Brotli libraries (brotlienc / brotlidec).")
endif()

set(BROTLI_LIBRARIES ${BROTLIENC_LIB} ${BROTLIDEC_LIB})
include_directories(${BROTLI_INCLUDE_DIRS})

message(STATUS "Brotli include: ${BROTLI_INCLUDE_DIRS}")
message(STATUS "Brotli libs: ${BROTLI_LIBRARIES}")

# -----------------------------
# Python 模块
# -----------------------------
pybind11_add_module(trvl src/wrap.cpp) 
target_link_libraries(trvl PRIVATE ${OpenCV_LIBS} ${BROTLI_LIBRARIES})

# -----------------------------
# Example 可执行文件
# -----------------------------
add_executable(Example src/example.cpp)
target_link_libraries(Example PRIVATE ${OpenCV_LIBS} ${BROTLI_LIBRARIES})


