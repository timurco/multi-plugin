cmake_minimum_required(VERSION 3.20)
project(MultiPlugin VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_AE_PLUGINS "Build After Effects plugins" ON)
option(BUILD_OFX_PLUGINS "Build OpenFX plugins" ON)
option(BUILD_EXAMPLES "Build example plugins" ON)
option(BUILD_TESTS "Build tests" OFF)

# Platform detection
if(APPLE)
    set(MP_PLATFORM_MAC ON)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64") # Universal binary
elseif(WIN32)
    set(MP_PLATFORM_WIN ON)
else()
    set(MP_PLATFORM_LINUX ON)
endif()

# Version management
set(MP_VERSION_MAJOR 0)
set(MP_VERSION_MINOR 1)
set(MP_VERSION_PATCH 0)
set(MP_VERSION_STAGE 0) # PF_Stage_DEVELOP

# Auto-increment build number
if(EXISTS "${CMAKE_BINARY_DIR}/build_number.txt")
    file(READ "${CMAKE_BINARY_DIR}/build_number.txt" MP_VERSION_BUILD)
    math(EXPR MP_VERSION_BUILD "${MP_VERSION_BUILD} + 1")
else()
    set(MP_VERSION_BUILD 1)
endif()
file(WRITE "${CMAKE_BINARY_DIR}/build_number.txt" "${MP_VERSION_BUILD}")

message(STATUS "MultiPlugin version: ${MP_VERSION_MAJOR}.${MP_VERSION_MINOR}.${MP_VERSION_PATCH} build ${MP_VERSION_BUILD}")

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find SDKs using environment variables
if(BUILD_AE_PLUGINS)
    include(FindAESDK)
    if(NOT AESDK_FOUND)
        message(WARNING "AE SDK not found, disabling AE plugin build")
        set(BUILD_AE_PLUGINS OFF)
    endif()
endif()

if(BUILD_OFX_PLUGINS)
    include(FindOFX)
    if(NOT OFX_FOUND)
        message(WARNING "OFX not found, disabling OFX plugin build")
        set(BUILD_OFX_PLUGINS OFF)
    endif()
endif()

# Generate version source file
configure_file(
    "${CMAKE_SOURCE_DIR}/src/core/version.cpp.in"
    "${CMAKE_BINARY_DIR}/generated/version.cpp"
    @ONLY
)

# Core library (header-only interface)
add_library(multiplugin-core INTERFACE)
target_include_directories(multiplugin-core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(multiplugin-core INTERFACE cxx_std_17)

# Version library (recompiled only when version changes)
add_library(multiplugin-version STATIC
    ${CMAKE_BINARY_DIR}/generated/version.cpp
)
target_include_directories(multiplugin-version PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(multiplugin-version PRIVATE
    MP_VERSION_MAJOR=${MP_VERSION_MAJOR}
    MP_VERSION_MINOR=${MP_VERSION_MINOR}
    MP_VERSION_PATCH=${MP_VERSION_PATCH}
    MP_VERSION_STAGE=${MP_VERSION_STAGE}
    MP_VERSION_BUILD=${MP_VERSION_BUILD}
)

# Platform-specific definitions
if(MP_PLATFORM_MAC)
    target_compile_definitions(multiplugin-core INTERFACE MP_PLATFORM_MAC)
elseif(MP_PLATFORM_WIN)
    target_compile_definitions(multiplugin-core INTERFACE MP_PLATFORM_WIN _WINDOWS)
    # For parallel execution policy
    target_compile_options(multiplugin-core INTERFACE /std:c++17)
else()
    target_compile_definitions(multiplugin-core INTERFACE MP_PLATFORM_LINUX)
endif()

# Include plugin helper
include(MPPlugin)

# Add subdirectories
add_subdirectory(src)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()