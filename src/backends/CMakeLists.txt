# Backend implementations

if(BUILD_AE_PLUGINS)
    add_subdirectory(ae)
endif()

if(BUILD_OFX_PLUGINS)
    add_subdirectory(ofx)
endif()