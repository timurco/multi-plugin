# Example plugins

# Add Invert example
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/invert/CMakeLists.txt")
    add_subdirectory(invert)
endif()

# Future examples can be added here
# add_subdirectory(blur)
# add_subdirectory(glow)
# add_subdirectory(keyer)