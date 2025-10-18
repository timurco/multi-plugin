/**
 * @file helpers.hpp
 * @brief Helper functions for creating parameters with fluent API
 *
 * All functions return builders that support method chaining
 */

#pragma once

#include "param_builder.hpp"

namespace mp {

/**
 * @brief Create a float slider parameter
 * @return FloatSliderBuilder supporting .percent(), .precision(), .sliderRange()
 *
 * Example:
 *   mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f)
 *       .percent()
 *       .precision(2)
 */
template<class Bag>
auto floatSlider(unsigned int disk_id, float Bag::*member, const char* name,
                float min, float max, float default_val) {
    return FloatSliderBuilder<Bag>(disk_id, member, name, min, max, default_val);
}

/**
 * @brief Create an integer slider parameter
 * @return IntSliderBuilder supporting .sliderRange()
 *
 * Example:
 *   mp::intSlider(1, &Params::count, "Count", 1, 1000, 100)
 *       .sliderRange(1, 100)
 */
template<class Bag, class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
auto intSlider(unsigned int disk_id, T Bag::*member, const char* name,
              int min, int max, int default_val) {
    return IntSliderBuilder<Bag, T>(disk_id, member, name, min, max, default_val);
}

/**
 * @brief Create a checkbox parameter
 *
 * Example:
 *   mp::checkbox(2, &Params::enabled, "Enabled", true)
 */
template<class Bag>
auto checkbox(unsigned int disk_id, bool Bag::*member, const char* name,
             bool default_val = false) {
    return CheckboxBuilder<Bag>(disk_id, member, name, default_val);
}

/**
 * @brief Create a flag checkbox parameter (operates on bit flags)
 *
 * Example:
 *   mp::flagCheckbox(3, &Params::flags, FLAG_GLOW_ENABLED, "Glow Enabled", false)
 */
template<class Bag>
auto flagCheckbox(unsigned int disk_id, uint32_t Bag::*member,
                 uint32_t flag_mask, const char* name, bool default_val = false) {
    return FlagCheckboxBuilder<Bag>(disk_id, member, flag_mask, name, default_val);
}

/**
 * @brief Create a button parameter
 * @return ButtonBuilder supporting .onClick()
 *
 * Example:
 *   mp::button<MyParams>(4, "Randomize", "Generate random values")
 *       .onClick([](auto& ctx) {
 *           ctx.setValue(&MyParams::seed, rand());
 *       })
 */
template<class Bag>
auto button(unsigned int disk_id, const char* name, const char* label) {
    return ButtonBuilder<Bag>(disk_id, name, label);
}

/**
 * @brief Create a popup/dropdown parameter
 *
 * Example:
 *   mp::popup(5, &Params::mode, "Mode", "Fast|Quality|Extreme", 0)
 */
template<class Bag, class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
auto popup(unsigned int disk_id, T Bag::*member, const char* name,
          const char* items, int default_index = 0) {
    return PopupBuilder<Bag, T>(disk_id, member, name, items, default_index);
}

/**
 * @brief Create an angle parameter (stored as radians)
 *
 * Example:
 *   mp::angle(6, &Params::rotation, "Rotation", 0.0f)
 */
template<class Bag>
auto angle(unsigned int disk_id, float Bag::*member, const char* name,
          float default_val = 0.0f) {
    return AngleBuilder<Bag>(disk_id, member, name, default_val);
}

/**
 * @brief Create a 2D point parameter
 *
 * Example:
 *   mp::point2D(7, &Params::center, "Center", 0.0f, 0.0f)
 */
template<class Bag>
auto point2D(unsigned int disk_id, float (Bag::*member)[2], const char* name,
            float default_x = 0.0f, float default_y = 0.0f) {
    return Point2DBuilder<Bag>(disk_id, member, name, default_x, default_y);
}

/**
 * @brief Create a color parameter (RGBA 0-1 range)
 *
 * Example:
 *   mp::color(8, &Params::tint, "Tint", {1.0f, 0.8f, 0.3f, 1.0f})
 */
template<class Bag>
auto color(unsigned int disk_id, float (Bag::*member)[4], const char* name,
          const float (&default_color)[4]) {
    return ColorBuilder<Bag>(disk_id, member, name, default_color);
}

} // namespace mp
