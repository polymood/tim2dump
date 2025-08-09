cmake_minimum_required(VERSION 3.20)
project(tim2dump VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(SOURCES
        src/main.cpp
        src/tim2_parser.cpp
        src/image_converter.cpp
        src/table_formatter.cpp
)

# Create executable
add_executable(tim2dump ${SOURCES})

# Include directories
target_include_directories(tim2dump PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(tim2dump PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Optional: Enable optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT MSVC)
        target_compile_options(tim2dump PRIVATE -O3)
    endif()
endif()