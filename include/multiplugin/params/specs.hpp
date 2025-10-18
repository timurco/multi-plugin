/**
 * @file specs.hpp
 * @brief Parameter specification types for MultiPlugin
 * Based on UParams.hpp architecture
 */

#pragma once

#include <cstdint>
#include <string>

namespace mp {

/**
 * @brief Float slider parameter specification
 */
struct SpecFloat {
    const char* name;
    float min;
    float max;
    float slider_min;
    float slider_max;
    float default_val;
    int precision;
    bool is_percent;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Integer slider parameter specification
 */
struct SpecInt {
    const char* name;
    int min;
    int max;
    int slider_min;
    int slider_max;
    int default_val;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Color parameter specification (RGBA 0-1 range)
 */
struct SpecColor {
    const char* name;
    float default_color[4];
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Boolean checkbox parameter specification
 */
struct SpecBool {
    const char* name;
    bool default_val;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Button parameter specification
 */
struct SpecButton {
    const char* name;
    const char* label;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Popup/dropdown parameter specification
 */
struct SpecPopup {
    const char* name;
    const char* items;  // Pipe-separated: "Item1|Item2|Item3"
    int default_index;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Angle parameter specification (stored as radians)
 */
struct SpecAngle {
    const char* name;
    float default_val;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief 2D point parameter specification
 */
struct SpecPoint2D {
    const char* name;
    float default_x;
    float default_y;
    unsigned int disk_id;
    std::string unique_name;
};

/**
 * @brief Bit flag parameter specification
 */
struct SpecFlag {
    const char* name;
    bool default_val;
    unsigned int disk_id;
    uint32_t flag_mask;
    std::string unique_name;
};

/**
 * @brief Group start/end specification
 */
struct SpecGroup {
    const char* name;
    bool is_start;
    unsigned int disk_id;
    std::string unique_name;
};

#ifdef BUILD_FOR_AE
/**
 * @brief Custom UI parameter specification (AE only)
 */
struct SpecCustom {
    const char* name;
    int width;
    int height;
    unsigned int disk_id;
    std::string unique_name;
};
#endif

/**
 * @brief Generate unique parameter name from disk_id and UI name
 */
inline std::string genUniqueName(unsigned int disk_id, const char* ui_name) {
    return "param_" + std::to_string(disk_id);
}

} // namespace mp
