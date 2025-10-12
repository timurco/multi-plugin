# Core sources
file(GLOB_RECURSE CORE_SOURCES
    core/*.cpp
)

# Exclude generated version file from glob (it's added separately)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*version\\.cpp$")

# Create core implementation library if we have sources
if(CORE_SOURCES)
    add_library(multiplugin-core-impl STATIC ${CORE_SOURCES})
    target_link_libraries(multiplugin-core-impl PUBLIC
        multiplugin-core
        multiplugin-version
    )
endif()
