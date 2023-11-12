cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(ExampleGraphics)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(.. ${CMAKE_CURRENT_BINARY_DIR}/gfxbase_dir)

add_executable(host_compute host_compute.cpp)
target_link_libraries(host_compute gfxbase)
