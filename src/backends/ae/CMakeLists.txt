# After Effects backend library

# Create AE backend as OBJECT library (for linking into plugins)
add_library(multiplugin_ae_backend OBJECT
    ae_backend.cpp
)

target_include_directories(multiplugin_ae_backend PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(multiplugin_ae_backend PUBLIC
    multiplugin_core
    AESDK::Core
    AESDK::Util
)

# Set compile definitions for AE
target_compile_definitions(multiplugin_ae_backend PRIVATE
    PLUGIN_NAME="${PLUGIN_NAME}"
    AE_MATCH_NAME="${AE_MATCH_NAME}"
    PLUGIN_CATEGORY="${PLUGIN_CATEGORY}"
    AE_RESERVED_INFO="${AE_RESERVED_INFO}"
    PLUGIN_SUPPORT_URL="${PLUGIN_SUPPORT_URL}"
)