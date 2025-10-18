/**
 * @file ofx_param_impl.hpp
 * @brief OpenFX parameter implementation
 */

#pragma once

#ifdef BUILD_FOR_OFX

#include "param_set.hpp"
#include "sources.hpp"

namespace mp {

/**
 * @brief OpenFX parameter builder (runtime - just fetches existing params)
 * Parameters are created during describe phase by backend
 * Uses pure C API
 */
class OFXParamBuilder {
    OfxParamSetHandle param_set_;
    const OfxParameterSuiteV1* param_suite_;

public:
    OFXParamBuilder(OfxParamSetHandle param_set, const OfxParameterSuiteV1* suite)
        : param_set_(param_set), param_suite_(suite) {}

    /**
     * @brief Fetch float slider parameter
     */
    HFloat floatSlider(unsigned int disk_id, const std::string& unique_name,
                      const char* name, float min, float max,
                      float slider_min, float slider_max, float default_val,
                      int precision, bool is_percent) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HFloat{param};
    }

    /**
     * @brief Fetch integer slider parameter
     */
    HInt intSlider(unsigned int disk_id, const std::string& unique_name,
                  const char* name, int min, int max,
                  int slider_min, int slider_max, int default_val) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HInt{param};
    }

    /**
     * @brief Fetch color parameter
     */
    HColor color(unsigned int disk_id, const std::string& unique_name,
                const char* name, const float (&default_color)[4]) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HColor{param};
    }

    /**
     * @brief Fetch checkbox parameter
     */
    HBool checkbox(unsigned int disk_id, const std::string& unique_name,
                  const char* name, bool default_val) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HBool{param};
    }

    /**
     * @brief Fetch button parameter
     */
    HButton button(unsigned int disk_id, const std::string& unique_name,
                  const char* name, const char* label) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HButton{param};
    }

    /**
     * @brief Fetch popup parameter
     */
    HPopup popup(unsigned int disk_id, const std::string& unique_name,
                const char* name, const char* items, int default_index) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HPopup{param};
    }

    /**
     * @brief Fetch angle parameter
     */
    HAngle angle(unsigned int disk_id, const std::string& unique_name,
                const char* name, float default_val) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HAngle{param};
    }

    /**
     * @brief Fetch 2D point parameter
     */
    HPoint2D point2D(unsigned int disk_id, const std::string& unique_name,
                    const char* name, float default_x, float default_y) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HPoint2D{param};
    }

    /**
     * @brief Fetch group parameter
     */
    HGroup groupStart(unsigned int disk_id, const std::string& unique_name,
                     const char* name) {
        OfxParamHandle param = nullptr;
        param_suite_->paramGetHandle(param_set_, unique_name.c_str(), &param, nullptr);
        return HGroup{param};
    }

    /**
     * @brief Group end (no-op for OFX)
     */
    HGroup groupEnd(unsigned int disk_id) {
        return HGroup{nullptr};
    }
};

// Implementation of ParamSet member functions for OFX

template<class Bag, class... Ps>
template<class Builder, class P>
void ParamSet<Bag, Ps...>::buildOne(Builder& builder, P& param) {
    if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
        param.handle = builder.floatSlider(param.spec.disk_id, param.spec.unique_name,
                                          param.spec.name, param.spec.min, param.spec.max,
                                          param.spec.slider_min, param.spec.slider_max,
                                          param.spec.default_val, param.spec.precision,
                                          param.spec.is_percent);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
        param.handle = builder.intSlider(param.spec.disk_id, param.spec.unique_name,
                                        param.spec.name, param.spec.min, param.spec.max,
                                        param.spec.slider_min, param.spec.slider_max,
                                        param.spec.default_val);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
        param.handle = builder.color(param.spec.disk_id, param.spec.unique_name,
                                    param.spec.name, param.spec.default_color);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
        param.handle = builder.checkbox(param.spec.disk_id, param.spec.unique_name,
                                       param.spec.name, param.spec.default_val);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
        param.handle = builder.checkbox(param.spec.disk_id, param.spec.unique_name,
                                       param.spec.name, param.spec.default_val);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecButton>) {
        param.handle = builder.button(param.spec.disk_id, param.spec.unique_name,
                                     param.spec.name, param.spec.label);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
        param.handle = builder.popup(param.spec.disk_id, param.spec.unique_name,
                                    param.spec.name, param.spec.items,
                                    param.spec.default_index);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
        param.handle = builder.angle(param.spec.disk_id, param.spec.unique_name,
                                    param.spec.name, param.spec.default_val);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
        param.handle = builder.point2D(param.spec.disk_id, param.spec.unique_name,
                                      param.spec.name, param.spec.default_x,
                                      param.spec.default_y);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecGroup>) {
        if (param.spec.is_start) {
            param.handle = builder.groupStart(param.spec.disk_id, param.spec.unique_name,
                                            param.spec.name);
        } else {
            param.handle = builder.groupEnd(param.spec.disk_id);
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

    if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
        double value;
        if (source.pullDouble(param.spec.unique_name, &value)) return 1;
        if (param.spec.is_percent) {
            value *= 0.01;
        }
        bag.*(param.member) = static_cast<float>(value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
        int value;
        if (source.pullInt(param.spec.unique_name, &value)) return 1;
        bag.*(param.member) = static_cast<typename P::val_type>(value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
        double r, g, b, a;
        if (source.pullColor(param.spec.unique_name, &r, &g, &b, &a)) return 1;
        auto& c = bag.*(param.member);
        c[0] = static_cast<float>(r);
        c[1] = static_cast<float>(g);
        c[2] = static_cast<float>(b);
        c[3] = static_cast<float>(a);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
        bool value;
        if (source.pullBool(param.spec.unique_name, &value)) return 1;
        bag.*(param.member) = value;
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
        bool value;
        if (source.pullBool(param.spec.unique_name, &value)) return 1;
        if (value) {
            bag.*(param.member) |= param.spec.flag_mask;
        } else {
            bag.*(param.member) &= ~param.spec.flag_mask;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
        int value;
        if (source.pullChoice(param.spec.unique_name, &value)) return 1;
        bag.*(param.member) = static_cast<typename P::val_type>(value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
        double value;
        if (source.pullDouble(param.spec.unique_name, &value)) return 1;
        bag.*(param.member) = static_cast<float>(value * M_PI / 180.0); // Convert degrees to radians
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
        double x, y;
        if (source.pullDouble2D(param.spec.unique_name, &x, &y)) return 1;
        auto& pt = bag.*(param.member);
        pt[0] = static_cast<float>(x);
        pt[1] = static_cast<float>(y);
    }

    return 0;
}

template<class Bag, class... Ps>
template<class Source, class P, class T>
int ParamSet<Bag, Ps...>::setValueOne(const Source& source, const P& param,
                                     T Bag::*member_ptr, const T& value) const {
    // Check if this parameter matches
    if constexpr (!std::is_same_v<typename P::val_type, std::nullptr_t>) {
        if constexpr (std::is_same_v<decltype(param.member), decltype(member_ptr)>) {
            if (param.member != member_ptr) {
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
        if (param.spec.is_percent) {
            v *= 100.0;
        }
        if (param.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(param.handle.param, v);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
        if (param.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(param.handle.param, static_cast<int>(value));
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
        if constexpr (std::is_same_v<T, float[4]>) {
            const auto& c = static_cast<const float(&)[4]>(value);
            if (param.handle.param && source.param_suite) {
                double r = c[0], g = c[1], b = c[2], a = c[3];
                source.param_suite->paramSetValue(param.handle.param, r, g, b, a);
                return 0;
            }
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
        if (param.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(param.handle.param, value ? 1 : 0);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
        bool flag_value = value & param.spec.flag_mask;
        if (param.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(param.handle.param, flag_value ? 1 : 0);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
        if (param.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(param.handle.param, static_cast<int>(value));
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
        double degrees = static_cast<double>(value) * 180.0 / M_PI;
        if (param.handle.param && source.param_suite) {
            source.param_suite->paramSetValue(param.handle.param, degrees);
            return 0;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
        if constexpr (std::is_same_v<T, float[2]>) {
            const auto& pt = static_cast<const float(&)[2]>(value);
            if (param.handle.param && source.param_suite) {
                double x = pt[0], y = pt[1];
                source.param_suite->paramSetValue(param.handle.param, x, y);
                return 0;
            }
        }
    }

    return 1;
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::setValueOne(const Source& source, const P& param, const Bag& bag) const {
    if constexpr (!std::is_same_v<typename P::val_type, std::nullptr_t>) {
        return setValueOne(source, param, param.member, bag.*(param.member));
    }
    return 0;
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::setVisibleOne(const Source& source, const P& param, bool visible) const {
    if (param.handle.param && source.param_suite) {
        // setIsSecret is inverse of setVisible
        source.param_suite->paramSetSecret(param.handle.param, !visible ? 1 : 0);
        return 0;
    }
    return 1;
}

template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::setEnabledOne(const Source& source, const P& param, bool enabled) const {
    if (param.handle.param && source.param_suite) {
        source.param_suite->paramSetEnabled(param.handle.param, enabled ? 1 : 0);
        return 0;
    }
    return 1;
}

} // namespace mp

#endif // BUILD_FOR_OFX
