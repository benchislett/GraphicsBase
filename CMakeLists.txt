cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+

# Load Dependencies

## Raylib
set(RAYLIB_VERSION 4.5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

## ImGui
FetchContent_Populate(imgui
  URL https://github.com/ocornut/imgui/archive/docking.zip
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/imgui
)

file(GLOB IMGUI_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/misc/cpp/imgui_stdlib.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/backends/imgui_impl_opengl2.cpp
)

## Raylib-ImGui

FetchContent_Populate(rlImGui
  GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
  GIT_TAG main
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/rlImGui
)

file(GLOB RLIMGUI_SRCS ${CMAKE_CURRENT_BINARY_DIR}/rlImGui/*.cpp)

add_library(gfxbase STATIC ${IMGUI_SRCS} ${RLIMGUI_SRCS})
target_link_libraries(gfxbase raylib)

target_include_directories(gfxbase PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/imgui
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/backends
  ${CMAKE_CURRENT_BINARY_DIR}/imgui/misc/cpp
)

target_include_directories(gfxbase PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(gfxbase PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/rlImGui)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
  target_link_libraries(gfxbase "-framework IOKit")
  target_link_libraries(gfxbase "-framework Cocoa")
  target_link_libraries(gfxbase "-framework OpenGL")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(gfxbase PUBLIC "-fsanitize=address" "-fno-omit-frame-pointer")
    target_link_options(gfxbase PUBLIC "-fsanitize=address")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(gfxbase PUBLIC "-march=native")
    target_link_options(gfxbase PUBLIC "-flto")
endif()

target_compile_options(gfxbase PRIVATE "-Wall" "-Wextra" "-Wpedantic")