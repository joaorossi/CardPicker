cmake_minimum_required(VERSION 3.16)
project(CardPicker LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# -------------------------------------------------------------------
# CPM.cmake setup
# -------------------------------------------------------------------
set(CPM_DOWNLOAD_VERSION 0.42.0)
if(NOT CPM_SOURCE_CACHE)
  set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpmcache")
endif()
if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
  message(STATUS "Downloading CPM.cmake...")
  file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
    "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
    TLS_VERIFY ON)
endif()
include(${CMAKE_BINARY_DIR}/cmake/CPM.cmake)

# -------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------

# raylib
CPMAddPackage("gh:raysan5/raylib#5.5")

# OpenCV needs to be downloaded from https://github.com/opencv/opencv/releases
# Unzip it at a known location and set the appropriate path here
# Only version 4.12.0 was tested
set(OPENCV_INSTALL_DIR "C:\\Users\\joaor\\dev\\opencv")
find_package(OpenCV REQUIRED CONFIG PATHS "${OPENCV_INSTALL_DIR}\\build\\x64\\vc16\\lib")

# -------------------------------------------------------------------
# Main executable
# -------------------------------------------------------------------
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib ${OpenCV_LIBS})

# Pass MediaMTX path to C++
if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    MEDIAMTX_PATH="${MEDIAMTX_DIR}/mediamtx.exe"
  )
endif()

# -------------------------------------------------------------------
# Post build copy of OpenCV dlls
# -------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${OPENCV_INSTALL_DIR}\\build\\x64\\vc16\\lib"
        "${OPENCV_INSTALL_DIR}\\build\\x64\\vc16\\bin"
        "${CMAKE_BINARY_DIR}")

