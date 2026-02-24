cmake_minimum_required(VERSION 3.20)

project(FIM
    VERSION 0.1.0
    DESCRIPTION "File Integrity Monitor"
    LANGUAGES CXX
)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Automatically find all source files in src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# Define the executable
add_executable(FIM
    ${SRC_FILES}
)

# Include directories
target_include_directories(FIM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compilation options
if(MSVC)
    target_compile_options(FIM PRIVATE /W4 /permissive-)
else()
    target_compile_options(FIM PRIVATE -Wall -Wextra -Wpedantic)
endif()
