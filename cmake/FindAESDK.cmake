# FindAESDK.cmake
# Find Adobe After Effects SDK
#
# This module uses the AE_SDK_PATH environment variable to locate the SDK
#
# Sets:
#   AESDK_FOUND - True if AE SDK found
#   AESDK_INCLUDE_DIRS - Include directories
#   AESDK_UTIL_SOURCES - Utility source files
#   AESDK_PIPL_TOOL - Path to PiPLtool executable

# Check environment variable
if(NOT DEFINED ENV{AE_SDK_PATH})
    message(WARNING "AE_SDK_PATH environment variable not set!")
    message(WARNING "Please set: export AE_SDK_PATH=/path/to/adobe-ae-sdk-2025")
    set(AESDK_FOUND FALSE)
    return()
endif()

set(AE_SDK_PATH $ENV{AE_SDK_PATH})

# Normalize path
file(TO_CMAKE_PATH "${AE_SDK_PATH}" AE_SDK_PATH)

# Verify SDK exists
if(NOT EXISTS "${AE_SDK_PATH}/Examples/Headers/AE_Effect.h")
    message(WARNING "AE SDK not found at ${AE_SDK_PATH}")
    message(WARNING "Looking for: ${AE_SDK_PATH}/Examples/Headers/AE_Effect.h")
    set(AESDK_FOUND FALSE)
    return()
endif()

message(STATUS "Found AE SDK at: ${AE_SDK_PATH}")

# Set include directories
set(AESDK_INCLUDE_DIRS
    "${AE_SDK_PATH}/Examples/Headers"
    "${AE_SDK_PATH}/Examples/Headers/SP"
    "${AE_SDK_PATH}/Examples/Util"
    "${AE_SDK_PATH}/Examples/Resources"
)

# Collect utility sources
file(GLOB AESDK_UTIL_SOURCES
    "${AE_SDK_PATH}/Examples/Util/*.cpp"
    "${AE_SDK_PATH}/Examples/Util/*.c"
)

# Exclude platform-specific files if needed
if(NOT WIN32)
    list(FILTER AESDK_UTIL_SOURCES EXCLUDE REGEX ".*Win.*")
    list(FILTER AESDK_UTIL_SOURCES EXCLUDE REGEX ".*DirectX.*")  # DirectX is Windows-only
endif()
if(NOT APPLE)
    list(FILTER AESDK_UTIL_SOURCES EXCLUDE REGEX ".*Mac.*")
    list(FILTER AESDK_UTIL_SOURCES EXCLUDE REGEX ".*Cocoa.*")
endif()

# Find PiPLtool
if(WIN32)
    set(AESDK_PIPL_TOOL "${AE_SDK_PATH}/Examples/Resources/PiPLtool.exe")
    if(NOT EXISTS "${AESDK_PIPL_TOOL}")
        message(WARNING "PiPLtool.exe not found at ${AESDK_PIPL_TOOL}")
    endif()
elseif(APPLE)
    # On Mac, we use Rez instead
    find_program(REZ_COMMAND Rez PATHS /usr/bin)
    if(REZ_COMMAND)
        set(AESDK_PIPL_TOOL "${REZ_COMMAND}")
    else()
        message(WARNING "Rez command not found")
    endif()
endif()

# Check Premiere compatibility
if(EXISTS "${AE_SDK_PATH}/Examples/Headers/PrSDKPixelFormat.h")
    message(STATUS "Found Premiere Pro SDK headers")
    set(AESDK_PREMIERE_SUPPORT TRUE)
else()
    set(AESDK_PREMIERE_SUPPORT FALSE)
endif()

# Create imported target
if(NOT TARGET AESDK::Core)
    add_library(AESDK::Core INTERFACE IMPORTED)
    set_target_properties(AESDK::Core PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AESDK_INCLUDE_DIRS}"
    )

    # Platform-specific compile definitions
    if(APPLE)
        set_property(TARGET AESDK::Core APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS AE_OS_MAC
        )
    elseif(WIN32)
        set_property(TARGET AESDK::Core APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS AE_OS_WIN
        )
    endif()
endif()

# Create utility library target (without :: in name)
if(NOT TARGET AESDK_Util AND AESDK_UTIL_SOURCES)
    add_library(AESDK_Util STATIC ${AESDK_UTIL_SOURCES})
    target_include_directories(AESDK_Util PUBLIC ${AESDK_INCLUDE_DIRS})

    if(APPLE)
        target_compile_definitions(AESDK_Util PRIVATE AE_OS_MAC)
        # Suppress warnings in SDK code
        target_compile_options(AESDK_Util PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-parameter
            -Wno-unused-variable
        )
    elseif(WIN32)
        target_compile_definitions(AESDK_Util PRIVATE AE_OS_WIN)
        target_compile_options(AESDK_Util PRIVATE
            /wd4996  # Disable deprecation warnings
            /wd4100  # Unreferenced formal parameter
        )
    endif()

    # Create alias for consistent naming
    if(NOT TARGET AESDK::Util)
        add_library(AESDK::Util ALIAS AESDK_Util)
    endif()
endif()

set(AESDK_FOUND TRUE)