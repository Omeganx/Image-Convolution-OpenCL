cmake_minimum_required(VERSION 3.10)
project(ImageConvo)
add_compile_options(-lm)
find_package(OpenCL REQUIRED)

include_directories(headers)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY})

add_compile_definitions(CL_TARGET_OPENCL_VERSION=120)
file(GLOB SRC_FILES src/*.c headers/*.h)
add_executable(imageconvo ${SRC_FILES})
target_link_libraries(imageconvo -lm ${OpenCL_LIBRARY})
