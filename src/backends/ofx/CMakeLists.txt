# OpenFX backend library

# Create OFX backend as OBJECT library (for linking into plugins)
add_library(multiplugin_ofx_backend OBJECT
    ofx_backend.cpp
)

target_include_directories(multiplugin_ofx_backend PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(multiplugin_ofx_backend PUBLIC
    multiplugin_core
    OFX::Core
)

# Set compile definitions for OFX
target_compile_definitions(multiplugin_ofx_backend PRIVATE
    PLUGIN_UNIQUE_ID="${PLUGIN_UNIQUE_ID}"
)