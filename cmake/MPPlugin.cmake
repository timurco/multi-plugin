# MPPlugin.cmake
# Helper function to create multi-host plugins

set(_MP_AE_OUT_FLAG_MAP
    "KEEP_RESOURCE_OPEN=0x00000001"
    "WIDE_TIME_INPUT=0x00000002"
    "NON_PARAM_VARY=0x00000004"
    "RESERVED6=0x00000008"
    "SEQUENCE_DATA_NEEDS_FLATTENING=0x00000010"
    "I_DO_DIALOG=0x00000020"
    "USE_OUTPUT_EXTENT=0x00000040"
    "SEND_DO_DIALOG=0x00000080"
    "DISPLAY_ERROR_MESSAGE=0x00000100"
    "I_EXPAND_BUFFER=0x00000200"
    "PIX_INDEPENDENT=0x00000400"
    "I_WRITE_INPUT_BUFFER=0x00000800"
    "I_SHRINK_BUFFER=0x00001000"
    "WORKS_IN_PLACE=0x00002000"
    "RESERVED8=0x00004000"
    "CUSTOM_UI=0x00008000"
    "RESERVED7=0x00010000"
    "REFRESH_UI=0x00020000"
    "NOP_RENDER=0x00040000"
    "I_USE_SHUTTER_ANGLE=0x00080000"
    "I_USE_AUDIO=0x00100000"
    "I_AM_OBSOLETE=0x00200000"
    "FORCE_RERENDER=0x00400000"
    "PIPL_OVERRIDES_OUTDATA_OUTFLAGS=0x00800000"
    "I_HAVE_EXTERNAL_DEPENDENCIES=0x01000000"
    "DEEP_COLOR_AWARE=0x02000000"
    "SEND_UPDATE_PARAMS_UI=0x04000000"
    "AUDIO_FLOAT_ONLY=0x08000000"
    "AUDIO_IIR=0x10000000"
    "I_SYNTHESIZE_AUDIO=0x20000000"
    "AUDIO_EFFECT_TOO=0x40000000"
    "AUDIO_EFFECT_ONLY=0x80000000"
)

set(_MP_AE_OUT_FLAG2_MAP
    "SUPPORTS_QUERY_DYNAMIC_FLAGS=0x00000001"
    "I_USE_3D_CAMERA=0x00000002"
    "I_USE_3D_LIGHTS=0x00000004"
    "PARAM_GROUP_START_COLLAPSED_FLAG=0x00000008"
    "I_AM_THREADSAFE=0x00000010"
    "CAN_COMBINE_WITH_DESTINATION=0x00000020"
    "DOESNT_NEED_EMPTY_PIXELS=0x00000040"
    "REVEALS_ZERO_ALPHA=0x00000080"
    "PRESERVES_FULLY_OPAQUE_PIXELS=0x00000100"
    "SUPPORTS_SMART_RENDER=0x00000400"
    "RESERVED9=0x00000800"
    "FLOAT_COLOR_AWARE=0x00001000"
    "I_USE_COLORSPACE_ENUMERATION=0x00002000"
    "I_AM_DEPRECATED=0x00004000"
    "PPRO_DO_NOT_CLONE_SEQUENCE_DATA_FOR_RENDER=0x00008000"
    "RESERVED10=0x00010000"
    "AUTOMATIC_WIDE_TIME_INPUT=0x00020000"
    "I_USE_TIMECODE=0x00040000"
    "DEPENDS_ON_UNREFERENCED_MASKS=0x00080000"
    "OUTPUT_IS_WATERMARKED=0x00100000"
    "I_MIX_GUID_DEPENDENCIES=0x00200000"
    "AE13_5_THREADSAFE=0x00400000"
    "SUPPORTS_GET_FLATTENED_SEQUENCE_DATA=0x00800000"
    "CUSTOM_UI_ASYNC_MANAGER=0x01000000"
    "SUPPORTS_GPU_RENDER_F32=0x02000000"
    "RESERVED12=0x04000000"
    "SUPPORTS_THREADED_RENDERING=0x08000000"
    "MUTABLE_RENDER_SEQUENCE_DATA_SLOWER=0x10000000"
)

function(_mp_compute_flag_mask result_var base_value enable_list disable_list map_list)
    set(_value "${base_value}")

    set(_enable_upper "")
    foreach(item ${enable_list})
        if(NOT item STREQUAL "")
            string(TOUPPER "${item}" item_upper)
            list(APPEND _enable_upper "${item_upper}")
        endif()
    endforeach()

    set(_disable_upper "")
    foreach(item ${disable_list})
        if(NOT item STREQUAL "")
            string(TOUPPER "${item}" item_upper)
            list(APPEND _disable_upper "${item_upper}")
        endif()
    endforeach()

    foreach(pair ${map_list})
        if(pair MATCHES "=")
            string(REPLACE "=" ";" parts "${pair}")
            list(LENGTH parts parts_len)
            if(parts_len GREATER 1)
                list(GET parts 0 flag_name)
                list(GET parts 1 flag_value)
                string(TOUPPER "${flag_name}" flag_upper)
                list(FIND _enable_upper "${flag_upper}" idx)
                if(idx GREATER -1)
                    math(EXPR _value "${_value} | ${flag_value}")
                endif()
            endif()
        endif()
    endforeach()

    foreach(pair ${map_list})
        if(pair MATCHES "=")
            string(REPLACE "=" ";" parts "${pair}")
            list(LENGTH parts parts_len)
            if(parts_len GREATER 1)
                list(GET parts 0 flag_name)
                list(GET parts 1 flag_value)
                string(TOUPPER "${flag_name}" flag_upper)
                list(FIND _disable_upper "${flag_upper}" idx)
                if(idx GREATER -1)
                    math(EXPR _value "${_value} & ~${flag_value}")
                endif()
            endif()
        endif()
    endforeach()

    set(${result_var} "${_value}" PARENT_SCOPE)
endfunction()

# Main function to add a plugin
function(mp_add_plugin)
    set(options "")
    set(oneValueArgs
        NAME
        VERSION
        VENDOR
        CATEGORY
        DESCRIPTION
        AE_MATCH_NAME
        OFX_IDENTIFIER
        SUPPORT_URL
        AE_RESERVED_INFO
        VERSION_STAGE
        VERSION_BUILD
        BUNDLE_IDENTIFIER_AE
        BUNDLE_IDENTIFIER_OFX
        AE_INFO_FLAGS
        AE_OUT_FLAGS
        AE_OUT_FLAGS2
    )
    set(multiValueArgs
        SOURCES
        HOSTS
        DEFINITIONS
        LIBRARIES
        AE_OUT_FLAGS_ENABLE
        AE_OUT_FLAGS_DISABLE
        AE_OUT_FLAGS2_ENABLE
        AE_OUT_FLAGS2_DISABLE
    )

    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT PLUGIN_NAME)
        message(FATAL_ERROR "mp_add_plugin requires NAME to be specified")
    endif()

    # Set defaults
    if(NOT PLUGIN_HOSTS)
        set(PLUGIN_HOSTS AE OFX)
    endif()

    if(NOT PLUGIN_VERSION)
        if(DEFINED MP_VERSION_MAJOR AND DEFINED MP_VERSION_MINOR AND DEFINED MP_VERSION_PATCH)
            set(PLUGIN_VERSION "${MP_VERSION_MAJOR}.${MP_VERSION_MINOR}.${MP_VERSION_PATCH}")
        else()
            set(PLUGIN_VERSION "0.1.0")
        endif()
    endif()

    if(NOT PLUGIN_VENDOR)
        set(PLUGIN_VENDOR "MultiPlugin")
    endif()

    if(NOT PLUGIN_CATEGORY)
        set(PLUGIN_CATEGORY "MultiPlugin")
    endif()

    if(NOT PLUGIN_DESCRIPTION)
        set(PLUGIN_DESCRIPTION "${PLUGIN_NAME}")
    endif()

    if(NOT PLUGIN_AE_MATCH_NAME)
        set(PLUGIN_AE_MATCH_NAME "MP_${PLUGIN_NAME}")
    endif()

    if(NOT PLUGIN_SUPPORT_URL)
        set(PLUGIN_SUPPORT_URL "https://github.com/multiplugin")
    endif()

    if(NOT PLUGIN_AE_RESERVED_INFO)
        set(PLUGIN_AE_RESERVED_INFO 0)
    endif()

    if(NOT PLUGIN_AE_INFO_FLAGS)
        set(PLUGIN_AE_INFO_FLAGS 0)
    endif()

    set(_mp_base_out_flags 0)
    if(DEFINED PLUGIN_AE_OUT_FLAGS AND NOT "${PLUGIN_AE_OUT_FLAGS}" STREQUAL "")
        set(_mp_base_out_flags "${PLUGIN_AE_OUT_FLAGS}")
    endif()
    if(PLUGIN_AE_OUT_FLAGS_ENABLE)
        set(_mp_enable_list "${PLUGIN_AE_OUT_FLAGS_ENABLE}")
    elseif("${PLUGIN_AE_OUT_FLAGS}" STREQUAL "")
        set(_mp_enable_list "DEEP_COLOR_AWARE;PIX_INDEPENDENT;NON_PARAM_VARY")
    else()
        set(_mp_enable_list "")
    endif()
    _mp_compute_flag_mask(_mp_resolved_out_flags "${_mp_base_out_flags}" "${_mp_enable_list}" "${PLUGIN_AE_OUT_FLAGS_DISABLE}" "${_MP_AE_OUT_FLAG_MAP}")
    set(PLUGIN_AE_OUT_FLAGS "${_mp_resolved_out_flags}")

    set(_mp_base_out_flags2 0)
    if(DEFINED PLUGIN_AE_OUT_FLAGS2 AND NOT "${PLUGIN_AE_OUT_FLAGS2}" STREQUAL "")
        set(_mp_base_out_flags2 "${PLUGIN_AE_OUT_FLAGS2}")
    endif()
    if(PLUGIN_AE_OUT_FLAGS2_ENABLE)
        set(_mp_enable_list2 "${PLUGIN_AE_OUT_FLAGS2_ENABLE}")
    elseif("${PLUGIN_AE_OUT_FLAGS2}" STREQUAL "")
        set(_mp_enable_list2 "FLOAT_COLOR_AWARE;SUPPORTS_SMART_RENDER;SUPPORTS_THREADED_RENDERING")
    else()
        set(_mp_enable_list2 "")
    endif()
    _mp_compute_flag_mask(_mp_resolved_out_flags2 "${_mp_base_out_flags2}" "${_mp_enable_list2}" "${PLUGIN_AE_OUT_FLAGS2_DISABLE}" "${_MP_AE_OUT_FLAG2_MAP}")
    set(PLUGIN_AE_OUT_FLAGS2 "${_mp_resolved_out_flags2}")

    # Version components
    string(REPLACE "." ";" _mp_plugin_version_list "${PLUGIN_VERSION}")
    list(LENGTH _mp_plugin_version_list _mp_plugin_version_len)
    if(_mp_plugin_version_len GREATER 0)
        list(GET _mp_plugin_version_list 0 PLUGIN_VERSION_MAJOR)
    else()
        set(PLUGIN_VERSION_MAJOR 0)
    endif()
    if(_mp_plugin_version_len GREATER 1)
        list(GET _mp_plugin_version_list 1 PLUGIN_VERSION_MINOR)
    else()
        set(PLUGIN_VERSION_MINOR 0)
    endif()
    if(_mp_plugin_version_len GREATER 2)
        list(GET _mp_plugin_version_list 2 PLUGIN_VERSION_PATCH)
    else()
        set(PLUGIN_VERSION_PATCH 0)
    endif()

    # Handle development stage (accept numeric or named values)
    set(_mp_stage_default 0)
    if(DEFINED MP_VERSION_STAGE)
        set(_mp_stage_default "${MP_VERSION_STAGE}")
    endif()
    set(_mp_stage_input "${_mp_stage_default}")
    if(DEFINED PLUGIN_VERSION_STAGE AND NOT "${PLUGIN_VERSION_STAGE}" STREQUAL "")
        set(_mp_stage_input "${PLUGIN_VERSION_STAGE}")
    endif()
    string(TOUPPER "${_mp_stage_input}" _mp_stage_upper)
    if(_mp_stage_upper STREQUAL "DEVELOP")
        set(_mp_stage_value 0)
    elseif(_mp_stage_upper STREQUAL "ALPHA")
        set(_mp_stage_value 1)
    elseif(_mp_stage_upper STREQUAL "BETA")
        set(_mp_stage_value 2)
    elseif(_mp_stage_upper STREQUAL "RELEASE")
        set(_mp_stage_value 3)
    else()
        set(_mp_stage_value "${_mp_stage_input}")
    endif()
    set(PLUGIN_VERSION_STAGE "${_mp_stage_value}")

    # Build number (falls back to auto-incremented framework build)
    if(DEFINED PLUGIN_VERSION_BUILD AND NOT "${PLUGIN_VERSION_BUILD}" STREQUAL "")
        set(_mp_build_value "${PLUGIN_VERSION_BUILD}")
    elseif(DEFINED MP_VERSION_BUILD)
        set(_mp_build_value "${MP_VERSION_BUILD}")
    else()
        set(_mp_build_value 1)
    endif()
    set(PLUGIN_VERSION_BUILD "${_mp_build_value}")

    # Slug helpers for identifiers
    set(_mp_vendor_slug "${PLUGIN_VENDOR}")
    string(REPLACE " " "" _mp_vendor_slug "${_mp_vendor_slug}")
    string(REPLACE "-" "" _mp_vendor_slug "${_mp_vendor_slug}")
    string(TOLOWER "${_mp_vendor_slug}" _mp_vendor_slug)

    set(_mp_name_slug "${PLUGIN_NAME}")
    string(REPLACE " " "" _mp_name_slug "${_mp_name_slug}")
    string(REPLACE "-" "" _mp_name_slug "${_mp_name_slug}")
    string(TOLOWER "${_mp_name_slug}" _mp_name_slug)

    if(NOT PLUGIN_OFX_IDENTIFIER)
        set(PLUGIN_OFX_IDENTIFIER "com.${_mp_vendor_slug}.ofx.${_mp_name_slug}")
    endif()

    if(NOT PLUGIN_BUNDLE_IDENTIFIER_AE)
        set(PLUGIN_BUNDLE_IDENTIFIER_AE "com.${_mp_vendor_slug}.ae.${_mp_name_slug}")
    endif()

    if(NOT PLUGIN_BUNDLE_IDENTIFIER_OFX)
        set(PLUGIN_BUNDLE_IDENTIFIER_OFX "${PLUGIN_OFX_IDENTIFIER}")
    endif()

    message(STATUS "Configuring plugin: ${PLUGIN_NAME} v${PLUGIN_VERSION}")

    # Build AE version
    if("AE" IN_LIST PLUGIN_HOSTS AND BUILD_AE_PLUGINS)
        _mp_add_ae_plugin()
    endif()

    # Build OFX version
    if("OFX" IN_LIST PLUGIN_HOSTS AND BUILD_OFX_PLUGINS)
        _mp_add_ofx_plugin()
    endif()
endfunction()

# Helper function for AE plugin
function(_mp_add_ae_plugin)
    set(TARGET_NAME "${PLUGIN_NAME}_AE")

    # Create plugin library
    add_library(${TARGET_NAME} MODULE ${PLUGIN_SOURCES})
    target_sources(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/backends/ae/ae_backend.cpp
    )

    # Link libraries
    target_link_libraries(${TARGET_NAME} PRIVATE
        multiplugin-core
        multiplugin-version
        AESDK::Core
        AESDK::Util
        ${PLUGIN_LIBRARIES}
    )

    # Compile definitions
    target_compile_definitions(${TARGET_NAME} PRIVATE
        BUILD_FOR_AE
        PLUGIN_NAME="${PLUGIN_NAME}"
        PLUGIN_VERSION="${PLUGIN_VERSION}"
        PLUGIN_VENDOR="${PLUGIN_VENDOR}"
        PLUGIN_CATEGORY="${PLUGIN_CATEGORY}"
        AE_MATCH_NAME="${PLUGIN_AE_MATCH_NAME}"
        PLUGIN_DESCRIPTION="${PLUGIN_DESCRIPTION}"
        PLUGIN_SUPPORT_URL="${PLUGIN_SUPPORT_URL}"
        AE_RESERVED_INFO=${PLUGIN_AE_RESERVED_INFO}
        AE_INFO_FLAGS=${PLUGIN_AE_INFO_FLAGS}
        AE_OUT_FLAGS=${PLUGIN_AE_OUT_FLAGS}
        AE_OUT_FLAGS2=${PLUGIN_AE_OUT_FLAGS2}
        PLUGIN_VERSION_MAJOR=${PLUGIN_VERSION_MAJOR}
        PLUGIN_VERSION_MINOR=${PLUGIN_VERSION_MINOR}
        PLUGIN_VERSION_PATCH=${PLUGIN_VERSION_PATCH}
        PLUGIN_VERSION_STAGE=${PLUGIN_VERSION_STAGE}
        PLUGIN_VERSION_BUILD=${PLUGIN_VERSION_BUILD}
        PLUGIN_BUNDLE_IDENTIFIER="${PLUGIN_BUNDLE_IDENTIFIER_AE}"
        ${PLUGIN_DEFINITIONS}
    )

    # Platform-specific settings
    if(APPLE)
        # macOS bundle structure
        set(_mp_info_plist "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_Info.plist")
        configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Info.plist.ae.in"
            "${_mp_info_plist}"
            @ONLY
        )
        set_target_properties(${TARGET_NAME} PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "plugin"
            OUTPUT_NAME "${PLUGIN_NAME}"
            MACOSX_BUNDLE_INFO_PLIST "${_mp_info_plist}"
        )

        # Ensure AE entry points are exported from the bundle
        target_link_options(${TARGET_NAME} PRIVATE
            "LINKER:-exported_symbol,_EffectMain"
            "LINKER:-exported_symbol,_PluginDataEntryFunction2"
        )

        # Generate and compile .r file for macOS
        _mp_generate_pipl_resource(${TARGET_NAME})

        # Installation path for AE plugin bundles
        set(_mp_install_dir "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/${PLUGIN_CATEGORY}")
        install(TARGETS ${TARGET_NAME}
            BUNDLE DESTINATION "${_mp_install_dir}"
            LIBRARY DESTINATION "${_mp_install_dir}"
        )

    elseif(WIN32)
        # Windows DLL
        set_target_properties(${TARGET_NAME} PROPERTIES
            SUFFIX ".aex"
            OUTPUT_NAME "${PLUGIN_NAME}"
        )

        # Generate and add RC file for Windows
        _mp_generate_windows_resource(${TARGET_NAME})
    endif()

    message(STATUS "  -> AE plugin: ${TARGET_NAME}")
endfunction()

# Helper function for OFX plugin
function(_mp_add_ofx_plugin)
    set(TARGET_NAME "${PLUGIN_NAME}_OFX")

    # Create plugin library
    add_library(${TARGET_NAME} MODULE ${PLUGIN_SOURCES})
    target_sources(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/backends/ofx/ofx_backend.cpp
    )

    # Link libraries
    target_link_libraries(${TARGET_NAME} PRIVATE
        multiplugin-core
        multiplugin-version
        OFX::Core
        ${PLUGIN_LIBRARIES}
    )

    # Add OFX Support if available
    if(TARGET OFX::Support)
        target_link_libraries(${TARGET_NAME} PRIVATE OFX::Support)
    endif()

    # Compile definitions
    target_compile_definitions(${TARGET_NAME} PRIVATE
        BUILD_FOR_OFX
        PLUGIN_NAME="${PLUGIN_NAME}"
        PLUGIN_VERSION="${PLUGIN_VERSION}"
        PLUGIN_VENDOR="${PLUGIN_VENDOR}"
        PLUGIN_CATEGORY="${PLUGIN_CATEGORY}"
        OFX_IDENTIFIER="${PLUGIN_OFX_IDENTIFIER}"
        PLUGIN_DESCRIPTION="${PLUGIN_DESCRIPTION}"
        PLUGIN_SUPPORT_URL="${PLUGIN_SUPPORT_URL}"
        PLUGIN_VERSION_MAJOR=${PLUGIN_VERSION_MAJOR}
        PLUGIN_VERSION_MINOR=${PLUGIN_VERSION_MINOR}
        PLUGIN_VERSION_PATCH=${PLUGIN_VERSION_PATCH}
        PLUGIN_VERSION_STAGE=${PLUGIN_VERSION_STAGE}
        PLUGIN_VERSION_BUILD=${PLUGIN_VERSION_BUILD}
        PLUGIN_BUNDLE_IDENTIFIER="${PLUGIN_BUNDLE_IDENTIFIER_OFX}"
        ${PLUGIN_DEFINITIONS}
    )

    # Platform-specific settings
    if(APPLE)
        # macOS bundle structure for OFX
        set(_mp_ofx_info_plist "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_Info.plist")
        configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Info.plist.ofx.in"
            "${_mp_ofx_info_plist}"
            @ONLY
        )
        set_target_properties(${TARGET_NAME} PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "ofx.bundle"
            OUTPUT_NAME "${PLUGIN_NAME}"
            MACOSX_BUNDLE_INFO_PLIST "${_mp_ofx_info_plist}"
        )

    elseif(WIN32)
        # Windows DLL for OFX
        set_target_properties(${TARGET_NAME} PROPERTIES
            SUFFIX ".ofx"
            OUTPUT_NAME "${PLUGIN_NAME}"
        )

    else()
        # Linux shared object
        set_target_properties(${TARGET_NAME} PROPERTIES
            SUFFIX ".ofx.bundle"
            PREFIX ""
            OUTPUT_NAME "${PLUGIN_NAME}"
        )
    endif()

    message(STATUS "  -> OFX plugin: ${TARGET_NAME}")
endfunction()

# Generate PiPL resource for macOS AE plugins
function(_mp_generate_pipl_resource TARGET)
    # Create plugin config JSON
    set(PLUGIN_CONFIG_JSON "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_config.json")
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/plugin_config.json.in"
        ${PLUGIN_CONFIG_JSON}
        @ONLY
    )

    # Generate .r file after target is built
    set(PIPL_R_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}PiPL.r")
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env python3
            "${CMAKE_SOURCE_DIR}/tools/generate_pipl.py"
            ${PLUGIN_CONFIG_JSON}
            ${PIPL_R_FILE}
        COMMENT "Generating PiPL resource file for ${TARGET}"
        VERBATIM
    )

    # Compile .r to .rsrc
    set(PIPL_RSRC_FILE "$<TARGET_BUNDLE_DIR:${TARGET}>/Contents/Resources/${PLUGIN_NAME}.rsrc")

    # Create Resources directory first
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${TARGET}>/Contents/Resources"
        COMMENT "Creating Resources directory for ${TARGET}"
    )

    # Then compile the resource
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND ${AESDK_PIPL_TOOL}
            -o ${PIPL_RSRC_FILE}
            -useDF
            -i "${AE_SDK_PATH}/Examples/Headers"
            -i "${AE_SDK_PATH}/Examples/Resources"
            ${PIPL_R_FILE}
        COMMENT "Compiling PiPL resource for ${TARGET}"
        VERBATIM
    )
endfunction()

# Generate Windows resource for AE plugins
function(_mp_generate_windows_resource TARGET)
    # Create plugin config JSON
    set(PLUGIN_CONFIG_JSON "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_config.json")
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/plugin_config.json.in"
        ${PLUGIN_CONFIG_JSON}
        @ONLY
    )

    # Generate .r file
    set(PIPL_R_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}PiPL.r")
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND python
            "${CMAKE_SOURCE_DIR}/tools/generate_pipl.py"
            ${PLUGIN_CONFIG_JSON}
            ${PIPL_R_FILE}
        COMMENT "Generating PiPL resource file for ${TARGET}"
    )

    # Convert .r to .rc using PiPLtool
    set(PIPL_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}PiPL.rc")
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND ${AESDK_PIPL_TOOL}
            ${PIPL_R_FILE}
            ${PIPL_RC_FILE}
        COMMENT "Converting PiPL to RC for ${TARGET}"
    )

    # Add RC file to target
    target_sources(${TARGET} PRIVATE ${PIPL_RC_FILE})
endfunction()
