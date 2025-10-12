#pragma once

#include <cstdint>

namespace mp {

/**
 * @brief Plugin version information
 */
struct Version {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t stage;  // PF_Stage_DEVELOP=0, ALPHA=1, BETA=2, RELEASE=3
    uint16_t build;
};

/**
 * @brief Get current plugin version
 * @note This function is defined in generated version.cpp
 *       Only that file needs recompilation when version changes
 */
const Version& getPluginVersion();

/**
 * @brief Get version in After Effects format
 * @return Version encoded as PF_VERSION macro result
 */
uint32_t getAEVersion();

/**
 * @brief Get version string for OFX
 * @return Version as "major.minor.patch" string
 */
const char* getOFXVersionString();

} // namespace mp