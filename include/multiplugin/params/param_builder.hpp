/**
 * @file param_builder.hpp
 * @brief Fluent API builders for parameter creation
 *
 * Provides method chaining for convenient parameter creation:
 *   mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f).percent().precision(2)
 */

#pragma once

#include "param.hpp"
#include "specs.hpp"
#include "context.hpp"
#include <functional>

namespace mp {

/**
 * @brief Fluent builder for float slider parameters
 */
template<class Bag>
class FloatSliderBuilder {
public:
    // Expose types for OFX implementation compatibility
    using spec_type = SpecFloat;
    using handle_type = HFloat;
    using val_type = float;
     
    // Public for detail::addParam access
    Param<Bag, SpecFloat, HFloat, float> param_;
    
    // Expose spec and handle for direct access
    const spec_type& spec = param_.spec;
    handle_type& handle = param_.handle;
    
    FloatSliderBuilder(unsigned int disk_id, float Bag::*member, const char* name,
                      float min, float max, float default_val)
        : param_{member, {name, min, max, min, max, default_val,
                         1 /* PF_Precision_TENTHS */, false /* is_percent */,
                         disk_id, genUniqueName(disk_id, name)}, {}} {}

    /**
     * @brief Enable percent display
     */
    FloatSliderBuilder& percent() {
        param_.spec.is_percent = true;
        return *this;
    }

    /**
     * @brief Set precision (0-4)
     * 0 = INTEGER, 1 = TENTHS, 2 = HUNDREDTHS, 3 = THOUSANDTHS, 4 = TEN_THOUSANDTHS
     */
    FloatSliderBuilder& precision(int prec) {
        param_.spec.precision = prec;
        return *this;
    }

    /**
     * @brief Set slider range (independent from value range)
     */
    FloatSliderBuilder& sliderRange(float slider_min, float slider_max) {
        param_.spec.slider_min = slider_min;
        param_.spec.slider_max = slider_max;
        return *this;
    }

    /**
     * @brief Convert to Param for use in makeSet
     */
    operator Param<Bag, SpecFloat, HFloat, float>() const { return param_; }
};

/**
 * @brief Fluent builder for integer slider parameters
 */
template<class Bag, class T>
class IntSliderBuilder {
public:
    // Public for detail::addParam access
    Param<Bag, SpecInt, HInt, T> param_;
    IntSliderBuilder(unsigned int disk_id, T Bag::*member, const char* name,
                    int min, int max, int default_val)
        : param_{member, {name, min, max, min, max, default_val,
                         disk_id, genUniqueName(disk_id, name)}, {}} {}

    /**
     * @brief Set slider range (independent from value range)
     */
    IntSliderBuilder& sliderRange(int slider_min, int slider_max) {
        param_.spec.slider_min = slider_min;
        param_.spec.slider_max = slider_max;
        return *this;
    }

    operator Param<Bag, SpecInt, HInt, T>() const { return param_; }
};

/**
 * @brief Fluent builder for checkbox parameters
 */
template<class Bag>
class CheckboxBuilder {
public:
    // Public for detail::addParam access
    Param<Bag, SpecBool, HBool, bool> param_;
    CheckboxBuilder(unsigned int disk_id, bool Bag::*member, const char* name,
                   bool default_val = false)
        : param_{member, {name, default_val, disk_id, genUniqueName(disk_id, name)}, {}} {}

    operator Param<Bag, SpecBool, HBool, bool>() const { return param_; }
};

/**
 * @brief Fluent builder for popup parameters
 */
template<class Bag, class T>
class PopupBuilder {
public:
    // Public for detail::addParam access
    Param<Bag, SpecPopup, HPopup, T> param_;
    PopupBuilder(unsigned int disk_id, T Bag::*member, const char* name,
                const char* items, int default_index = 0)
        : param_{member, {name, items, default_index, disk_id, genUniqueName(disk_id, name)}, {}} {}

    operator Param<Bag, SpecPopup, HPopup, T>() const { return param_; }
};

/**
 * @brief Fluent builder for color parameters
 */
template<class Bag>
class ColorBuilder {
    using Arr = float[4];
public:
    // Public for detail::addParam access
    Param<Bag, SpecColor, HColor, Arr> param_;
    ColorBuilder(unsigned int disk_id, float (Bag::*member)[4], const char* name,
                const float (&default_color)[4])
        : param_{member, {name, {default_color[0], default_color[1],
                                 default_color[2], default_color[3]},
                         disk_id, genUniqueName(disk_id, name)}, {}} {}

    operator Param<Bag, SpecColor, HColor, Arr>() const { return param_; }
};

/**
 * @brief Fluent builder for angle parameters
 */
template<class Bag>
class AngleBuilder {
public:
    // Public for detail::addParam access
    Param<Bag, SpecAngle, HAngle, float> param_;
    AngleBuilder(unsigned int disk_id, float Bag::*member, const char* name,
                float default_val = 0.0f)
        : param_{member, {name, default_val, disk_id, genUniqueName(disk_id, name)}, {}} {}

    operator Param<Bag, SpecAngle, HAngle, float>() const { return param_; }
};

/**
 * @brief Fluent builder for 2D point parameters
 */
template<class Bag>
class Point2DBuilder {
    using Arr = float[2];
public:
    // Public for detail::addParam access
    Param<Bag, SpecPoint2D, HPoint2D, Arr> param_;
    Point2DBuilder(unsigned int disk_id, float (Bag::*member)[2], const char* name,
                  float default_x = 0.0f, float default_y = 0.0f)
        : param_{member, {name, default_x, default_y, disk_id, genUniqueName(disk_id, name)}, {}} {}

    operator Param<Bag, SpecPoint2D, HPoint2D, Arr>() const { return param_; }
};

/**
 * @brief Fluent builder for flag checkbox parameters
 */
template<class Bag>
class FlagCheckboxBuilder {
public:
    // Public for detail::addParam access
    Param<Bag, SpecFlag, HBool, uint32_t> param_;
    FlagCheckboxBuilder(unsigned int disk_id, uint32_t Bag::*member,
                       uint32_t flag_mask, const char* name, bool default_val = false)
        : param_{member, {name, default_val, disk_id, flag_mask, genUniqueName(disk_id, name)}, {}} {}

    operator Param<Bag, SpecFlag, HBool, uint32_t>() const { return param_; }
};

/**
 * @brief Fluent builder for button parameters with callback support
 */
template<class Bag>
class ButtonBuilder {
public:
    // Public for detail::addParam access
    Param<Bag, SpecButton, HButton, std::nullptr_t> param_;
    std::function<void(ParamContext<Bag>&)> callback_;
    ButtonBuilder(unsigned int disk_id, const char* name, const char* label)
        : param_{nullptr, {name, label, disk_id, genUniqueName(disk_id, name)}, {}} {}

    /**
     * @brief Set callback for button click
     * @param callback Function called when button is clicked, receives ParamContext
     */
    ButtonBuilder& onClick(std::function<void(ParamContext<Bag>&)> callback) {
        callback_ = callback;
        return *this;
    }

    operator Param<Bag, SpecButton, HButton, std::nullptr_t>() const { return param_; }

    /**
     * @brief Get parameter (for ParamSet)
     */
    const Param<Bag, SpecButton, HButton, std::nullptr_t>& getParam() const { return param_; }

    /**
     * @brief Execute callback if set
     */
    void executeCallback(ParamContext<Bag>& ctx) const {
        if (callback_) {
            callback_(ctx);
        }
    }

    bool hasCallback() const { return static_cast<bool>(callback_); }

    unsigned int getDiskId() const { return param_.spec.disk_id; }
};

/**
 * @brief Fluent builder for group start
 */
inline auto groupBegin(unsigned int disk_id, const char* name) {
    return Param<void, SpecGroup, HGroup, std::nullptr_t>{
        nullptr,
        {name, true, disk_id, genUniqueName(disk_id, name)},
        {}
    };
}

/**
 * @brief Fluent builder for group end
 */
inline auto groupEnd(unsigned int disk_id) {
    return Param<void, SpecGroup, HGroup, std::nullptr_t>{
        nullptr,
        {"", false, disk_id, genUniqueName(disk_id, "")},
        {}
    };
}

} // namespace mp
