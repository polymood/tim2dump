cmake_minimum_required(VERSION 3.20)
project(tim2dump VERSION 1.0.0 LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Sources
set(SOURCES
        src/main.cpp
        src/tim2_parser.cpp
        src/image_converter.cpp
        src/table_formatter.cpp
)

# Executable
add_executable(tim2dump ${SOURCES})

# Include paths
target_include_directories(tim2dump PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# If stb_image_write.h exists, define its implementation in image_converter.cpp only.
# (Prevents multiple-definition problems if the header is included elsewhere.)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb_image_write.h")
    set_source_files_properties(
            src/image_converter.cpp
            PROPERTIES COMPILE_DEFINITIONS "STB_IMAGE_WRITE_IMPLEMENTATION"
    )
    # Optional: let code know PNG is available (if you want to #ifdef in code)
    target_compile_definitions(tim2dump PRIVATE TIM2DUMP_HAVE_STB=1)
endif()

# Platform-specific tweaks
if(WIN32)
    target_compile_definitions(tim2dump PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT MSVC)
        target_compile_options(tim2dump PRIVATE -O3)
        # On Linux, strip symbols in Release to shrink the binary
        if(UNIX AND NOT APPLE)
            add_link_options(-s)
        endif()
    endif()
endif()
