/**
 * @file ofx_param_impl.hpp
 * @brief OpenFX parameter implementation
 */

#pragma once

#ifdef BUILD_FOR_OFX

#include "param_set.hpp"
#include "sources.hpp"
#include <vector>
#include <string>
#include <cstring>

namespace mp {

/**
 * @brief OpenFX parameter builder for parameter creation (pure C API)
 * Used during describeInContext to define parameters
 * Analogous to OfxBuilder in UParams.hpp but using pure C API instead of Support Library
 */
class OFXParamBuilder {
    OfxParamSetHandle param_set_;
    const OfxParameterSuiteV1* param_suite_;
    const OfxPropertySuiteV1* prop_suite_;
    std::vector<std::string> grpStack_;  // Stack of group names for nesting

    /**
     * @brief Attach parameter to current group if inside one
     */
    void attachIfInside(OfxPropertySetHandle paramProps) {
        if (!grpStack_.empty()) {
            prop_suite_->propSetString(paramProps, kOfxParamPropParent, 0,
                                      grpStack_.back().c_str());
        }
    }

public:
    OFXParamBuilder(OfxParamSetHandle param_set,
                   const OfxParameterSuiteV1* param_suite,
                   const OfxPropertySuiteV1* prop_suite)
        : param_set_(param_set), param_suite_(param_suite), prop_suite_(prop_suite) {}

    /**
     * @brief Create float slider parameter
     */
    HFloat floatSlider(unsigned int disk_id, const std::string& unique_name,
                      const char* name, float min, float max,
                      float slider_min, float slider_max, float default_val,
                      int precision, bool is_percent) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeDouble,
                                 unique_name.c_str(), &paramProps);

        // Set properties
        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 0, default_val);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropMin, 0, min);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropMax, 0, max);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDisplayMin, 0, slider_min);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDisplayMax, 0, slider_max);

        // Set precision (digits after decimal)
        if (precision >= 0 && precision <= 4) {
            prop_suite_->propSetInt(paramProps, kOfxParamPropDigits, 0, precision);
        }

        // Set double type for percent display
        if (is_percent) {
            prop_suite_->propSetString(paramProps, kOfxParamPropDoubleType, 0,
                                      kOfxParamDoubleTypeScale);
        }

        attachIfInside(paramProps);
        return HFloat{nullptr};  // Handle not needed during define
    }

    /**
     * @brief Create integer slider parameter
     */
    HInt intSlider(unsigned int disk_id, const std::string& unique_name,
                  const char* name, int min, int max,
                  int slider_min, int slider_max, int default_val) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeInteger,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetInt(paramProps, kOfxParamPropDefault, 0, default_val);
        prop_suite_->propSetInt(paramProps, kOfxParamPropMin, 0, min);
        prop_suite_->propSetInt(paramProps, kOfxParamPropMax, 0, max);
        prop_suite_->propSetInt(paramProps, kOfxParamPropDisplayMin, 0, slider_min);
        prop_suite_->propSetInt(paramProps, kOfxParamPropDisplayMax, 0, slider_max);

        attachIfInside(paramProps);
        return HInt{nullptr};
    }

    /**
     * @brief Create color parameter
     */
    HColor color(unsigned int disk_id, const std::string& unique_name,
                const char* name, const float (&default_color)[4]) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeRGBA,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 0, default_color[0]);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 1, default_color[1]);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 2, default_color[2]);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 3, default_color[3]);

        attachIfInside(paramProps);
        return HColor{nullptr};
    }

    /**
     * @brief Create checkbox parameter
     */
    HBool checkbox(unsigned int disk_id, const std::string& unique_name,
                  const char* name, bool default_val) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeBoolean,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetInt(paramProps, kOfxParamPropDefault, 0, default_val ? 1 : 0);

        attachIfInside(paramProps);
        return HBool{nullptr};
    }

    /**
     * @brief Create button parameter
     */
    HButton button(unsigned int disk_id, const std::string& unique_name,
                  const char* name, const char* label) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypePushButton,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, label);
        prop_suite_->propSetString(paramProps, kOfxParamPropHint, 0, name);

        attachIfInside(paramProps);
        return HButton{nullptr};
    }

    /**
     * @brief Create popup parameter
     */
    HPopup popup(unsigned int disk_id, const std::string& unique_name,
                const char* name, const char* items, int default_index) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeChoice,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetInt(paramProps, kOfxParamPropDefault, 0, default_index);

        // Parse items string "opt1|opt2|opt3" and set choice options
        std::string itemsStr(items);
        int index = 0;
        size_t pos = 0;
        while (pos < itemsStr.length()) {
            size_t nextPos = itemsStr.find('|', pos);
            if (nextPos == std::string::npos) nextPos = itemsStr.length();

            std::string option = itemsStr.substr(pos, nextPos - pos);
            prop_suite_->propSetString(paramProps, kOfxParamPropChoiceOption, index, option.c_str());

            pos = nextPos + 1;
            index++;
        }

        attachIfInside(paramProps);
        return HPopup{nullptr};
    }

    /**
     * @brief Create angle parameter
     */
    HAngle angle(unsigned int disk_id, const std::string& unique_name,
                const char* name, float default_val) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeDouble,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetString(paramProps, kOfxParamPropDoubleType, 0,
                                  kOfxParamDoubleTypeAngle);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 0, default_val);

        attachIfInside(paramProps);
        return HAngle{nullptr};
    }

    /**
     * @brief Create 2D point parameter
     */
    HPoint2D point2D(unsigned int disk_id, const std::string& unique_name,
                    const char* name, float default_x, float default_y) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeDouble2D,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 0, default_x);
        prop_suite_->propSetDouble(paramProps, kOfxParamPropDefault, 1, default_y);

        attachIfInside(paramProps);
        return HPoint2D{nullptr};
    }

    /**
     * @brief Create group start parameter
     */
    HGroup groupStart(unsigned int disk_id, const std::string& unique_name,
                     const char* name) {
        OfxPropertySetHandle paramProps;
        param_suite_->paramDefine(param_set_, kOfxParamTypeGroup,
                                 unique_name.c_str(), &paramProps);

        prop_suite_->propSetString(paramProps, kOfxPropLabel, 0, name);
        prop_suite_->propSetInt(paramProps, kOfxParamPropGroupOpen, 0, 1);  // Open by default

        attachIfInside(paramProps);
        grpStack_.push_back(unique_name);  // Add to stack for nested groups
        return HGroup{nullptr};
    }

    /**
     * @brief Group end - pop from stack
     */
    HGroup groupEnd(unsigned int disk_id) {
        if (!grpStack_.empty()) {
            grpStack_.pop_back();
        }
        return HGroup{nullptr};
    }
};

// Helper trait to detect if type has param_ member (is a builder vs raw Param)
template<typename T, typename = void>
struct has_param_member : std::false_type {};

template<typename T>
struct has_param_member<T, std::void_t<decltype(std::declval<T>().param_)>> : std::true_type {};

// Implementation of ParamSet member functions for OFX

template<class Bag, class... Ps>
template<class Builder, class P>
void ParamSet<Bag, Ps...>::buildOne(Builder& builder, P& param) {
    // Check if P has param_ member (builder) or is raw Param
    if constexpr (has_param_member<P>::value) {
        // Builder type (FloatSliderBuilder, etc.) - access internal param_
        auto& p = param.param_;

        if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
            p.handle = builder.floatSlider(p.spec.disk_id, p.spec.unique_name,
                                           p.spec.name, p.spec.min, p.spec.max,
                                           p.spec.slider_min, p.spec.slider_max,
                                           p.spec.default_val, p.spec.precision,
                                           p.spec.is_percent);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
            p.handle = builder.intSlider(p.spec.disk_id, p.spec.unique_name,
                                         p.spec.name, p.spec.min, p.spec.max,
                                         p.spec.slider_min, p.spec.slider_max,
                                         p.spec.default_val);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
            p.handle = builder.color(p.spec.disk_id, p.spec.unique_name,
                                     p.spec.name, p.spec.default_color);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
            p.handle = builder.checkbox(p.spec.disk_id, p.spec.unique_name,
                                        p.spec.name, p.spec.default_val);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
            p.handle = builder.checkbox(p.spec.disk_id, p.spec.unique_name,
                                        p.spec.name, p.spec.default_val);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecButton>) {
            p.handle = builder.button(p.spec.disk_id, p.spec.unique_name,
                                      p.spec.name, p.spec.label);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
            p.handle = builder.popup(p.spec.disk_id, p.spec.unique_name,
                                     p.spec.name, p.spec.items,
                                     p.spec.default_index);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
            p.handle = builder.angle(p.spec.disk_id, p.spec.unique_name,
                                     p.spec.name, p.spec.default_val);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
            p.handle = builder.point2D(p.spec.disk_id, p.spec.unique_name,
                                       p.spec.name, p.spec.default_x,
                                       p.spec.default_y);
        }
    } else {
        // Raw Param (from groupBegin/groupEnd) - use directly
        auto& p = param;

        if constexpr (std::is_same_v<typename P::spec_type, SpecGroup>) {
            if (p.spec.is_start) {
                p.handle = builder.groupStart(p.spec.disk_id, p.spec.unique_name,
                                             p.spec.name);
            } else {
                p.handle = builder.groupEnd(p.spec.disk_id);
            }
        }
    }
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::fetchOne(const Source& source, Bag& bag, const P& param) const {
    // Skip parameters without values
    if constexpr (std::is_same_v<typename P::val_type, std::nullptr_t>) {
        return 0;
    }

    // Access param_ directly to avoid dangling reference
    const auto& p = param.param_;

    if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
        double value;
        if (source.pullDouble(p.spec.unique_name, &value)) return 1;
        if (p.spec.is_percent) {
            value *= 0.01;
        }
        bag.*(p.member) = static_cast<float>(value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
        int value;
        if (source.pullInt(p.spec.unique_name, &value)) return 1;
        bag.*(p.member) = static_cast<typename P::val_type>(value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
        double r, g, b, a;
        if (source.pullColor(p.spec.unique_name, &r, &g, &b, &a)) return 1;
        auto& c = bag.*(p.member);
        c[0] = static_cast<float>(r);
        c[1] = static_cast<float>(g);
        c[2] = static_cast<float>(b);
        c[3] = static_cast<float>(a);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
        bool value;
        if (source.pullBool(p.spec.unique_name, &value)) return 1;
        bag.*(p.member) = value;
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
        bool value;
        if (source.pullBool(p.spec.unique_name, &value)) return 1;
        if (value) {
            bag.*(p.member) |= p.spec.flag_mask;
        } else {
            bag.*(p.member) &= ~p.spec.flag_mask;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
        int value;
        if (source.pullChoice(p.spec.unique_name, &value)) return 1;
        bag.*(p.member) = static_cast<typename P::val_type>(value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
        double value;
        if (source.pullDouble(p.spec.unique_name, &value)) return 1;
        bag.*(p.member) = static_cast<float>(value * M_PI / 180.0); // Convert degrees to radians
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
        double x, y;
        if (source.pullDouble2D(p.spec.unique_name, &x, &y)) return 1;
        auto& pt = bag.*(p.member);
        pt[0] = static_cast<float>(x);
        pt[1] = static_cast<float>(y);
    }

    return 0;
}

template<class Bag, class... Ps>
template<class Source, class P, class T>
int ParamSet<Bag, Ps...>::setValueOne(const Source& source, const P& param,
                                     T Bag::*member_ptr, const T& value) const {
    // Access param_ directly to avoid dangling reference
    const auto& p = param.param_;

    // Check if this parameter matches
    if constexpr (!std::is_same_v<typename P::val_type, std::nullptr_t>) {
        if constexpr (std::is_same_v<decltype(p.member), decltype(member_ptr)>) {
            if (p.member != member_ptr) {
                return 1; // Continue searching
            }
        } else {
            return 1; // Type mismatch
        }
    } else {
        return 1; // Skip parameters without values
    }

    // Found the parameter, set its value using C API
    if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
        double v = static_cast<double>(value);
        if (p.spec.is_percent) {
            v *= 100.0;
        }
        if (p.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(p.handle.param, v);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
        if (p.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(p.handle.param, static_cast<int>(value));
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
        if constexpr (std::is_same_v<T, float[4]>) {
            const auto& c = static_cast<const float(&)[4]>(value);
            if (p.handle.param && source.param_suite) {
                double r = c[0], g = c[1], b = c[2], a = c[3];
                source.param_suite->paramSetValue(p.handle.param, r, g, b, a);
                return 0;
            }
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
        if (p.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(p.handle.param, value ? 1 : 0);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
        bool flag_value = value & p.spec.flag_mask;
        if (p.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(p.handle.param, flag_value ? 1 : 0);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
        if (p.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(p.handle.param, static_cast<int>(value));
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
        double degrees = static_cast<double>(value) * 180.0 / M_PI;
        if (p.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(p.handle.param, degrees);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
        if constexpr (std::is_same_v<T, float[2]>) {
            const auto& pt = static_cast<const float(&)[2]>(value);
            if (p.handle.param && source.param_suite) {
                double x = pt[0], y = pt[1];
                source.param_suite->paramSetValue(p.handle.param, x, y);
                return 0;
            }
        }
    }

    return 1;
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::setValueOne(const Source& source, const P& param, const Bag& bag) const {
    // Access param_ directly to avoid dangling reference
    const auto& p = param.param_;

    if constexpr (!std::is_same_v<typename P::val_type, std::nullptr_t>) {
        return setValueOne(source, param, p.member, bag.*(p.member));
    }
    return 0;
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::setVisibleOne(const Source& source, const P& param, bool visible) const {
    // Access param_ directly to avoid dangling reference
    const auto& p = param.param_;

    if (p.handle.param && source.param_suite) {
        // setIsSecret is inverse of setVisible
        source.param_suite->paramSetSecret(p.handle.param, !visible ? 1 : 0);
        return 0;
    }
    return 1;
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::setEnabledOne(const Source& source, const P& param, bool enabled) const {
    // Access param_ directly to avoid dangling reference
    const auto& p = param.param_;

    if (p.handle.param && source.param_suite) {
        source.param_suite->paramSetEnabled(p.handle.param, enabled ? 1 : 0);
        return 0;
    }
    return 1;
}

} // namespace mp

#endif // BUILD_FOR_OFX
