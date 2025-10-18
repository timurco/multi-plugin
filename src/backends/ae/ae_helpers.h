#pragma once

#include "multiplugin/core/logger.hpp"

#ifdef ERR
#undef ERR
#endif

#define ERR(FUNC)	do { if (!err) { err = (FUNC); if (err) LOG_ERR << "Error: " << GetErr(err); } } while (0)

#pragma once

#include <map> // for std::map
#include <string> // for std::string
#include <AE_Effect.h>
#include <A.h>
#include <SPErrorCodes.h>

inline const std::map<int, std::string> ERROR_NAMES = {
    // AE PF Errors (AE_Effect.h):
    { PF_Err_NONE, "PF_Err_NONE" },
    { PF_Err_OUT_OF_MEMORY, "PF_Err_OUT_OF_MEMORY" },
    { PF_Err_INTERNAL_STRUCT_DAMAGED, "PF_Err_INTERNAL_STRUCT_DAMAGED" },
    { PF_Err_INVALID_INDEX, "PF_Err_INVALID_INDEX" },
    { PF_Err_UNRECOGNIZED_PARAM_TYPE, "PF_Err_UNRECOGNIZED_PARAM_TYPE" },
    { PF_Err_INVALID_CALLBACK, "PF_Err_INVALID_CALLBACK" },
    { PF_Err_BAD_CALLBACK_PARAM, "PF_Err_BAD_CALLBACK_PARAM" },
    { PF_Interrupt_CANCEL, "PF_Interrupt_CANCEL" },
    { PF_Err_CANNOT_PARSE_KEYFRAME_TEXT, "PF_Err_CANNOT_PARSE_KEYFRAME_TEXT" }
};

static const std::map<int32_t, std::string> SP_ERR_NAMES = {
    // AE SP Errors (SPErrorCodes.h):
    { kASNoError, "kASNoError" },
    { kASUnimplementedError, "kASUnimplementedError" },
    { kASUserCanceledError, "kASUserCanceledError" },
    { kSPNoError, "kSPNoError" },
    { kSPUnimplementedError, "kSPUnimplementedError" },
    { kSPUserCanceledError, "kSPUserCanceledError" },
    { kSPOperationInterrupted, "kSPOperationInterrupted" },
    { kSPLogicError, "kSPLogicError" },
    { kSPCantAcquirePluginError, "kSPCantAcquirePluginError" },
    { kSPCantReleasePluginError, "kSPCantReleasePluginError" },
    { kSPPluginAlreadyReleasedError, "kSPPluginAlreadyReleasedError" },
    { kSPAdapterAlreadyExistsError, "kSPAdapterAlreadyExistsError" },
    { kSPBadAdapterListIteratorError, "kSPBadAdapterListIteratorError" },
    { kSPBadParameterError, "kSPBadParameterError" },
    { kSPCantChangeBlockDebugNowError, "kSPCantChangeBlockDebugNowError" },
    { kSPBlockDebugNotEnabledError, "kSPBlockDebugNotEnabledError" },
    { kSPOutOfMemoryError, "kSPOutOfMemoryError" },
    { kSPBlockSizeOutOfRangeError, "kSPBlockSizeOutOfRangeError" },
    { kSPPluginCachesFlushResponse, "kSPPluginCachesFlushResponse" },
    { kSPTroubleAddingFilesError, "kSPTroubleAddingFilesError" },
    { kSPBadFileListIteratorError, "kSPBadFileListIteratorError" },
    { kSPTroubleInitializingError, "kSPTroubleInitializingError" },
    { kHostCanceledStartupPluginsError, "kHostCanceledStartupPluginsError" },
    { kSPNotASweetPeaPluginError, "kSPNotASweetPeaPluginError" },
    { kSPAlreadyInSPCallerError, "kSPAlreadyInSPCallerError" },
    { kSPUnknownAdapterError, "kSPUnknownAdapterError" },
    { kSPBadPluginListIteratorError, "kSPBadPluginListIteratorError" },
    { kSPBadPluginHost, "kSPBadPluginHost" },
    { kSPCantAddHostPluginError, "kSPCantAddHostPluginError" },
    { kSPPluginNotFound, "kSPPluginNotFound" },
    { kSPCorruptPiPLError, "kSPCorruptPiPLError" },
    { kSPBadPropertyListIteratorError, "kSPBadPropertyListIteratorError" },
    { kSPSuiteNotFoundError, "kSPSuiteNotFoundError" },
    { kSPSuiteAlreadyExistsError, "kSPSuiteAlreadyExistsError" },
    { kSPSuiteAlreadyReleasedError, "kSPSuiteAlreadyReleasedError" },
    { kSPBadSuiteListIteratorError, "kSPBadSuiteListIteratorError" },
    { kSPBadSuiteInternalVersionError, "kSPBadSuiteInternalVersionError" }
};

const std::map<int, std::string> A_ERR_NAMES = {
    // A_Err:
    { A_Err_NONE, "A_Err_NONE" },
    { A_Err_GENERIC, "A_Err_GENERIC" },
    { A_Err_STRUCT, "A_Err_STRUCT" },
    { A_Err_PARAMETER, "A_Err_PARAMETER" },
    { A_Err_ALLOC, "A_Err_ALLOC" },
    { A_Err_WRONG_THREAD, "A_Err_WRONG_THREAD" },
    { A_Err_CONST_PROJECT_MODIFICATION, "A_Err_CONST_PROJECT_MODIFICATION" },
    { A_Err_MISSING_SUITE, "A_Err_MISSING_SUITE" },
    { A_Err_NOT_IN_CACHE_OR_COMPUTE_PENDING, "A_Err_NOT_IN_CACHE_OR_COMPUTE_PENDING" },
    { A_Err_PROJECT_LOAD_FATAL, "A_Err_PROJECT_LOAD_FATAL" },
    { A_Err_EFFECT_APPLY_FATAL, "A_Err_EFFECT_APPLY_FATAL" }
};

inline const std::array<std::string, 35> CMD_NAMES = {
    "PF_Cmd_ABOUT",
    "PF_Cmd_GLOBAL_SETUP",
    "PF_Cmd_UNUSED_0",
    "PF_Cmd_GLOBAL_SETDOWN",
    "PF_Cmd_PARAMS_SETUP",
    "PF_Cmd_SEQUENCE_SETUP",
    "PF_Cmd_SEQUENCE_RESETUP",
    "PF_Cmd_SEQUENCE_FLATTEN",
    "PF_Cmd_SEQUENCE_SETDOWN",
    "PF_Cmd_DO_DIALOG",
    "PF_Cmd_FRAME_SETUP",
    "PF_Cmd_RENDER",
    "PF_Cmd_FRAME_SETDOWN",
    "PF_Cmd_USER_CHANGED_PARAM",
    "PF_Cmd_UPDATE_PARAMS_UI",
    "PF_Cmd_EVENT",
    "PF_Cmd_GET_EXTERNAL_DEPENDENCIES",
    "PF_Cmd_COMPLETELY_GENERAL",
    "PF_Cmd_QUERY_DYNAMIC_FLAGS",
    "PF_Cmd_AUDIO_RENDER",
    "PF_Cmd_AUDIO_SETUP",
    "PF_Cmd_AUDIO_SETDOWN",
    "PF_Cmd_ARBITRARY_CALLBACK",
    "PF_Cmd_SMART_PRE_RENDER",
    "PF_Cmd_SMART_RENDER",
    "PF_Cmd_RESERVED1",
    "PF_Cmd_RESERVED2",
    "PF_Cmd_RESERVED3",
    "PF_Cmd_GET_FLATTENED_SEQUENCE_DATA",
    "PF_Cmd_TRANSLATE_PARAMS_TO_PREFS",
    "PF_Cmd_RESERVED4",
    "PF_Cmd_SMART_RENDER_GPU",
    "PF_Cmd_GPU_DEVICE_SETUP",
    "PF_Cmd_GPU_DEVICE_SETDOWN",
    "PF_Cmd_NUM_CMDS"
};

inline std::string GetPfErr(PF_Err err) {
    auto it = ERROR_NAMES.find(err);
    auto err_code = " (" + std::to_string(err) + ")";
    if (it != ERROR_NAMES.end()) return it->second + err_code;
    else
        return "PF_Err?UNKNOWN" + err_code;
}

inline std::string GetAErr(A_Err err) {
    auto it = A_ERR_NAMES.find(err);
    auto err_code = " (" + std::to_string(err) + ")";
    if (it != A_ERR_NAMES.end()) return it->second + err_code;
    else
        return "A_Err?UNKNOWN" + err_code;
}

static std::string FormatSPErr(int32_t err) {
    std::ostringstream oss;
    oss << "'" << static_cast<char>((err >> 24) & 0xFF) << static_cast<char>((err >> 16) & 0xFF) << static_cast<char>((err >> 8) & 0xFF)
        << static_cast<char>(err & 0xFF) << "'";
    return oss.str();
}

inline std::string GetSpErr(SPErr err) {
    std::ostringstream formatSPErr;
    formatSPErr << "'" << static_cast<char>((err >> 24) & 0xFF) << static_cast<char>((err >> 16) & 0xFF)
                << static_cast<char>((err >> 8) & 0xFF) << static_cast<char>(err & 0xFF) << "'";

    auto err_code = " (" + formatSPErr.str() + ")";
    auto it = SP_ERR_NAMES.find(err);
    if (it != SP_ERR_NAMES.end()) {
        return it->second + err_code;
    } else {
        return "SPErr?UNKNOWN" + err_code;
    }
}

inline std::string GetErr(int err) {
    // First, search for the error in ERROR_NAMES
    auto it_pf = ERROR_NAMES.find(err);
    if (it_pf != ERROR_NAMES.end()) { return GetPfErr(err); }

    // If not found in ERROR_NAMES, search in A_ERR_NAMES
    auto it_a = A_ERR_NAMES.find(err);
    if (it_a != A_ERR_NAMES.end()) { return GetAErr(err); }

    // If not found in A_ERR_NAMES, search in SP_ERR_NAMES
    auto it_sp = SP_ERR_NAMES.find(err);
    if (it_sp != SP_ERR_NAMES.end()) {
        // If found, return the corresponding error name with the error code
        return GetSpErr(err);
    }

    // If not found in either ERROR_NAMES or A_ERR_NAMES, return an unknown error message with the error code
    return "UNKNOWN_ERROR (" + std::to_string(err) + ")";
}
