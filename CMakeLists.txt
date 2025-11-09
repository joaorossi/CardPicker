cmake_minimum_required(VERSION 3.16)
project(CardPicker LANGUAGES C CXX)

# C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use MSVC static runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# FetchContent used for dependencies
include(FetchContent)

# Get raylib
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5)

# Downloads OpenCV static build for windows
# This is a 446MB download, so it may take a while
FetchContent_Declare(OpenCV
    URL https://github.com/joaorossi/opencv/releases/download/windows-static-build/opencv-windows-static.zip
    URL_HASH SHA256=f6824842b2259e7b977d49deb3fd2a2d86b9dbe6835976f660d24bb88fa0a3c6)

# Populate FetchContent targets
FetchContent_MakeAvailable(raylib OpenCV)

# OpenCV
find_package(OpenCV REQUIRED CONFIG PATHS "${opencv_SOURCE_DIR}/${CMAKE_BUILD_TYPE}")


#========================================================================================


# Main executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib ${OpenCV_LIBS})
