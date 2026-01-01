cmake_minimum_required(VERSION 3.16)
project(SoftwareRasterizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find GLM (installed via Homebrew)
find_package(glm REQUIRED)

# Source files
set(PIPELINE_SOURCES
    src/pipeline/vertex_processor.cpp
    src/pipeline/fragment_processor.cpp
    src/pipeline/rasterizer.cpp
    src/pipeline/clipper.cpp
)

set(CORE_SOURCES
    src/camera.cpp
    src/framebuffer.cpp
    src/model_loader.cpp
    src/output.cpp
    src/texture.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${PIPELINE_SOURCES}
    ${CORE_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link GLM
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native)
    endif()
endif()

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Create output directory in build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/output)
