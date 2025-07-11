cmake_minimum_required(VERSION 3.10)
project(MEC_System C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV REQUIRED opencv4)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/common
    ${OPENCV_INCLUDE_DIRS}
)

# Source files
file(GLOB_RECURSE COMMON_SOURCES "src/common/*.c")
file(GLOB_RECURSE VIDEO_SOURCES "src/video/*.c")
file(GLOB_RECURSE RADAR_SOURCES "src/radar/*.c")
file(GLOB_RECURSE FUSION_SOURCES "src/fusion/*.c")

# Libraries
add_library(mec_common STATIC ${COMMON_SOURCES})
add_library(mec_video STATIC ${VIDEO_SOURCES})
add_library(mec_radar STATIC ${RADAR_SOURCES})
add_library(mec_fusion STATIC ${FUSION_SOURCES})

# Main executable
add_executable(mec_system src/main.c)

# Link libraries
target_link_libraries(mec_system
    mec_common
    mec_video
    mec_radar
    mec_fusion
    ${OPENCV_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    m
)

# Install targets
install(TARGETS mec_system DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/mec)