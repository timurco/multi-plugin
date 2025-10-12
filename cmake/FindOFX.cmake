# FindOFX.cmake
# Find OpenFX headers and support library
#
# This module uses the OFX_PATH environment variable to locate OpenFX
#
# Sets:
#   OFX_FOUND - True if OFX found
#   OFX_INCLUDE_DIRS - Include directories
#   OFX_SUPPORT_SOURCES - Support library source files

# Check environment variable
if(NOT DEFINED ENV{OFX_PATH})
    message(WARNING "OFX_PATH environment variable not set!")
    message(WARNING "Please set: export OFX_PATH=/path/to/openfx")
    set(OFX_FOUND FALSE)
    return()
endif()

set(OFX_PATH $ENV{OFX_PATH})

# Normalize path
file(TO_CMAKE_PATH "${OFX_PATH}" OFX_PATH)

# Check for main header
if(NOT EXISTS "${OFX_PATH}/include/ofxCore.h")
    # Try alternative structure
    if(EXISTS "${OFX_PATH}/Support/include/ofxCore.h")
        set(OFX_INCLUDE_BASE "${OFX_PATH}/Support")
    else()
        message(WARNING "OFX headers not found at ${OFX_PATH}")
        message(WARNING "Looking for: ${OFX_PATH}/include/ofxCore.h")
        set(OFX_FOUND FALSE)
        return()
    endif()
else()
    set(OFX_INCLUDE_BASE "${OFX_PATH}")
endif()

message(STATUS "Found OFX at: ${OFX_INCLUDE_BASE}")

# Set include directories
set(OFX_INCLUDE_DIRS
    "${OFX_INCLUDE_BASE}/include"
)

# Check for Support library
if(EXISTS "${OFX_PATH}/Support/include/ofxsImageEffect.h")
    list(APPEND OFX_INCLUDE_DIRS "${OFX_PATH}/Support/include")

    # Collect support library sources
    file(GLOB OFX_SUPPORT_SOURCES
        "${OFX_PATH}/Support/Library/*.cpp"
    )

    # Remove test/example files if any
    list(FILTER OFX_SUPPORT_SOURCES EXCLUDE REGEX ".*test.*|.*Test.*|.*example.*|.*Example.*")

    message(STATUS "Found OFX Support library with ${CMAKE_CURRENT_LIST_FILE} source files")
else()
    message(STATUS "OFX Support library not found (optional)")
    set(OFX_SUPPORT_SOURCES "")
endif()

# Create imported target for OFX headers
if(NOT TARGET OFX::Core)
    add_library(OFX::Core INTERFACE IMPORTED)
    set_target_properties(OFX::Core PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OFX_INCLUDE_DIRS}"
    )

    # OFX uses some Unix functions on Windows
    if(WIN32)
        set_property(TARGET OFX::Core APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS _WINDOWS WIN32
        )
    endif()
endif()

# Create support library target if sources found (without :: in name)
if(OFX_SUPPORT_SOURCES AND NOT TARGET OFX_Support)
    add_library(OFX_Support STATIC ${OFX_SUPPORT_SOURCES})
    target_include_directories(OFX_Support PUBLIC ${OFX_INCLUDE_DIRS})

    # Platform-specific settings
    if(APPLE)
        target_compile_options(OFX_Support PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-parameter
        )
    elseif(WIN32)
        target_compile_definitions(OFX_Support PRIVATE _WINDOWS WIN32)
        target_compile_options(OFX_Support PRIVATE
            /wd4996  # Disable deprecation warnings
            /wd4100  # Unreferenced formal parameter
        )
    endif()

    # Create alias for consistent naming
    if(NOT TARGET OFX::Support)
        add_library(OFX::Support ALIAS OFX_Support)
    endif()
endif()

set(OFX_FOUND TRUE)