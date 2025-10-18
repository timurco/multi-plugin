/**
 * @file ae_param_impl.hpp
 * @brief After Effects backend implementation for parameter system
 *
 * Architecture matches UParams.hpp:
 * - commit() pattern for centralized parameter registration
 * - disk_id in d.uu.id for serialization stability
 * - Return param_index (next_ - 1) for runtime access
 * - Direct PF_ParamDef field manipulation
 */

#pragma once

#ifdef BUILD_FOR_AE

// CRITICAL: AE SDK headers MUST be included in the .cpp file BEFORE this header
// This header assumes all AE types and macros are already defined (PF_InData, PF_OutData, etc.)
// If you get "unknown type" errors, make sure your .cpp includes AE SDK headers first

#include "specs.hpp"
#include "param.hpp"
#include "param_builder.hpp"
#include "param_set.hpp"
#include "sources.hpp"
#include "context.hpp"
#include "helpers.hpp"

#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace mp {

/**
 * @brief After Effects parameter builder backend
 *
 * Architecture matches UParams.hpp for low-level compatibility
 */
class AEParamBuilder {
private:
    PF_InData* in_data_;
    PF_OutData* out_data_;
    int next_;  // Next parameter index (starts at 1, 0 is input layer)

    /**
     * @brief Common commit function for all parameters (matches UParams.hpp)
     *
     * - Sets disk_id in d.uu.id for AE serialization stability
     * - Returns param_index (next_ - 1) for runtime access
     * - Increments next_ counter
     *
     * @tparam Handle Handle type to return
     * @param d Filled PF_ParamDef structure
     * @param disk_id Stable disk ID for serialization
     * @return Handle containing param_index (not disk_id!)
     */
    template<typename Handle>
    Handle commit(PF_ParamDef& d, unsigned int disk_id) {
        // Use disk_id for stable parameter identification (AE serialization)
        d.uu.id = static_cast<A_long>(disk_id);

        // Add parameter using AE macro
        PF_ADD_PARAM(in_data_, -1, &d);

        ++next_;
        out_data_->num_params = next_;

        // Return param index (not disk_id!) - this is what gets stored in handle
        return Handle{next_ - 1};
    }

public:
    /**
     * @brief Constructor matching UParams.hpp
     * @param in_data PF_InData pointer
     * @param out_data PF_OutData pointer
     * @param first Number of input layers (default 0)
     */
    explicit AEParamBuilder(PF_InData* in_data, PF_OutData* out_data, int first = 0)
        : in_data_(in_data), out_data_(out_data), next_(first + 1) {
        out_data_->num_params = next_;  // Account for input layer(s)
    }

    /**
     * @brief Create float slider parameter (matches UParams.hpp lines 630-658)
     */
    HFloat floatSlider(unsigned int disk_id, const std::string& unique_name,
                      const char* name, float vmin, float vmax,
                      float smin, float smax, float def,
                      int prec, bool is_percent) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_FLOAT_SLIDER;
        PF_STRNNCPY(d.name, name, sizeof(d.name));

        // Direct field access matching UParams.hpp
        auto& fs = d.u.fs_d;
        fs.value       = def;
        fs.phase       = 0.0;
        fs.value_desc[0] = '\0';
        fs.valid_min   = vmin;
        fs.valid_max   = vmax;
        fs.slider_min  = smin;
        fs.slider_max  = smax;
        fs.dephault    = def;
        fs.precision   = static_cast<A_short>(prec);
        fs.display_flags = is_percent ? PF_ValueDisplayFlag_PERCENT : PF_ValueDisplayFlag_NONE;
        fs.fs_flags    = 0;
        fs.curve_tolerance = 0;
        fs.useExponent = FALSE;
        fs.exponent    = 0.0;

        return commit<HFloat>(d, disk_id);
    }

    /**
     * @brief Create color parameter (matches UParams.hpp lines 660-671)
     */
    HColor color(unsigned int disk_id, const std::string& unique_name,
                const char* name, const float c[4]) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_COLOR;
        PF_STRNNCPY(d.name, name, sizeof(d.name));

        d.u.cd.value = {
            static_cast<A_u_char>(c[3] * PF_MAX_CHAN8),  // Alpha
            static_cast<A_u_char>(c[0] * PF_MAX_CHAN8),  // Red
            static_cast<A_u_char>(c[1] * PF_MAX_CHAN8),  // Green
            static_cast<A_u_char>(c[2] * PF_MAX_CHAN8)   // Blue
        };
        d.u.cd.dephault = d.u.cd.value;

        return commit<HColor>(d, disk_id);
    }

    /**
     * @brief Create button parameter (matches UParams.hpp lines 673-679)
     */
    HButton button(unsigned int disk_id, const std::string& unique_name,
                  const char* name, const char* label) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_BUTTON;
        PF_STRNNCPY(d.name, label, sizeof(d.name));
        d.flags |= PF_ParamFlag_SUPERVISE;
        d.u.button_d.u.namesptr = name;

        return commit<HButton>(d, disk_id);
    }

    /**
     * @brief Create checkbox parameter (matches UParams.hpp lines 695-703)
     */
    HBool checkbox(unsigned int disk_id, const std::string& unique_name,
                  const char* name, bool def) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_CHECKBOX;
        PF_STRNNCPY(d.name, name, sizeof(d.name));
        d.u.bd.u.nameptr = "";
        d.u.bd.value     = def;
        d.u.bd.dephault  = def;

        return commit<HBool>(d, disk_id);
    }

    /**
     * @brief Create flag checkbox parameter (bitwise operations)
     */
    HFlag flagCheckbox(unsigned int disk_id, const std::string& unique_name,
                      const char* name, uint32_t mask, bool def) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_CHECKBOX;
        PF_STRNNCPY(d.name, name, sizeof(d.name));
        d.u.bd.u.nameptr = "";
        d.u.bd.value     = def;
        d.u.bd.dephault  = def;

        return HFlag{commit<HBool>(d, disk_id).id, mask};
    }

    /**
     * @brief Create integer slider parameter (matches UParams.hpp lines 707-717)
     */
    HInt intSlider(unsigned int disk_id, const std::string& unique_name,
                  const char* name, int vmin, int vmax,
                  int smin, int smax, int def) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_SLIDER;
        PF_STRNNCPY(d.name, name, sizeof(d.name));
        d.u.sd.valid_min  = vmin;
        d.u.sd.valid_max  = vmax;
        d.u.sd.slider_min = smin;
        d.u.sd.slider_max = smax;
        d.u.sd.value      = def;
        d.u.sd.dephault   = def;

        return commit<HInt>(d, disk_id);
    }

    /**
     * @brief Create popup/choice parameter (matches UParams.hpp lines 721-739)
     */
    HPopup popup(unsigned int disk_id, const std::string& unique_name,
                const char* name, const char* items, unsigned int defIdx) {

        PF_ParamDef d;
        AEFX_CLR_STRUCT(d);
        d.param_type = PF_Param_POPUP;
        PF_STRNNCPY(d.name, name, sizeof(d.name));
        d.flags |= PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY;

        // Count choices by parsing '|' separators
        int numChoices = 1;
        for (const char* c = items; *c; ++c) {
            if (*c == '|') ++numChoices;
        }

        d.u.pd.num_choices = static_cast<A_short>(numChoices);
        d.u.pd.dephault    = defIdx;
        d.u.pd.value       = defIdx;
        d.u.pd.u.namesptr  = items;

        return commit<HPopup>(d, disk_id);
    }

    /**
     * @brief Create angle parameter (matches UParams.hpp lines 743-752)
     */
    HAngle angle(unsigned int disk_id, const std::string& unique_name,
                const char* name, float def) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_ANGLE;
        PF_STRNNCPY(d.name, name, sizeof(d.name));

        const PF_Fixed fixed = static_cast<PF_Fixed>((def * 180.0f / M_PI) * 65536.0);
        d.u.ad.value    = fixed;
        d.u.ad.dephault = fixed;

        return commit<HAngle>(d, disk_id);
    }

    /**
     * @brief Create point2D parameter (matches UParams.hpp lines 756-767)
     */
    HPoint2D point2D(unsigned int disk_id, const std::string& unique_name,
                   const char* name, const float point[2]) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_POINT;
        PF_STRNNCPY(d.name, name, sizeof(d.name));
        d.u.td.restrict_bounds = FALSE;

        d.u.td.x_value = d.u.td.x_dephault = static_cast<int>(point[0]) << 16;
        d.u.td.y_value = d.u.td.y_dephault = static_cast<int>(point[1]) << 16;

        return commit<HPoint2D>(d, disk_id);
    }

    /**
     * @brief Begin parameter group (matches UParams.hpp lines 771-775)
     */
    HGroup groupStart(unsigned int disk_id, const std::string& unique_name,
                     const char* name) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_GROUP_START;
        PF_STRNNCPY(d.name, name, sizeof(d.name));

        return commit<HGroup>(d, disk_id);
    }

    /**
     * @brief End parameter group (matches UParams.hpp lines 779-783)
     */
    HGroup groupEnd(unsigned int disk_id) {

        PF_ParamDef d{{0}};
        d.param_type = PF_Param_GROUP_END;
        PF_STRNNCPY(d.name, "", sizeof(d.name));

        return commit<HGroup>(d, disk_id);
    }
};

// Helper namespace for parameter building - uses overload resolution like UParams.hpp
namespace detail {

// Float slider
template<class B, class Bag>
inline void addParam(B& b, FloatSliderBuilder<Bag>& p) {
    // Convert builder to Param via implicit conversion
    auto param = static_cast<Param<Bag, SpecFloat, HFloat, float>>(p);
    param.handle = b.floatSlider(param.spec.disk_id, param.spec.unique_name,
                                 param.spec.name, param.spec.min, param.spec.max,
                                 param.spec.slider_min, param.spec.slider_max,
                                 param.spec.default_val, param.spec.precision,
                                 param.spec.is_percent);
    // Copy handle back to builder's internal param
    p.param_.handle = param.handle;
}

// Checkbox
template<class B, class Bag>
inline void addParam(B& b, CheckboxBuilder<Bag>& p) {
    auto param = static_cast<Param<Bag, SpecBool, HBool, bool>>(p);
    param.handle = b.checkbox(param.spec.disk_id, param.spec.unique_name,
                             param.spec.name, param.spec.default_val);
    p.param_.handle = param.handle;
}

// Popup
template<class B, class Bag, class T>
inline void addParam(B& b, PopupBuilder<Bag, T>& p) {
    auto param = static_cast<Param<Bag, SpecPopup, HPopup, T>>(p);
    param.handle = b.popup(param.spec.disk_id, param.spec.unique_name,
                          param.spec.name, param.spec.items,
                          param.spec.default_index);
    p.param_.handle = param.handle;
}

// Integer slider
template<class B, class Bag, class T>
inline void addParam(B& b, IntSliderBuilder<Bag, T>& p) {
    auto param = static_cast<Param<Bag, SpecInt, HInt, T>>(p);
    param.handle = b.intSlider(param.spec.disk_id, param.spec.unique_name,
                              param.spec.name, param.spec.min, param.spec.max,
                              param.spec.slider_min, param.spec.slider_max,
                              param.spec.default_val);
    p.param_.handle = param.handle;
}

// Color
template<class B, class Bag>
inline void addParam(B& b, ColorBuilder<Bag>& p) {
    auto param = static_cast<Param<Bag, SpecColor, HColor, float[4]>>(p);
    param.handle = b.color(param.spec.disk_id, param.spec.unique_name,
                          param.spec.name, param.spec.default_color);
    p.param_.handle = param.handle;
}

// Button
template<class B, class Bag>
inline void addParam(B& b, ButtonBuilder<Bag>& p) {
    auto param = static_cast<Param<Bag, SpecButton, HButton, bool>>(p);
    param.handle = b.button(param.spec.disk_id, param.spec.unique_name,
                           param.spec.name, param.spec.label);
    p.param_.handle = param.handle;
}

// Angle
template<class B, class Bag>
inline void addParam(B& b, AngleBuilder<Bag>& p) {
    auto param = static_cast<Param<Bag, SpecAngle, HAngle, float>>(p);
    param.handle = b.angle(param.spec.disk_id, param.spec.unique_name,
                          param.spec.name, param.spec.default_val);
    p.param_.handle = param.handle;
}

// Point2D
template<class B, class Bag>
inline void addParam(B& b, Point2DBuilder<Bag>& p) {
    auto param = static_cast<Param<Bag, SpecPoint2D, HPoint2D, float[2]>>(p);
    param.handle = b.point2D(param.spec.disk_id, param.spec.unique_name,
                            param.spec.name, param.spec.default_xy);
    p.param_.handle = param.handle;
}

// Flag checkbox
template<class B, class Bag>
inline void addParam(B& b, FlagCheckboxBuilder<Bag>& p) {
    auto param = static_cast<Param<Bag, SpecFlag, HFlag, bool>>(p);
    param.handle = b.flagCheckbox(param.spec.disk_id, param.spec.unique_name,
                                 param.spec.name, param.spec.flag_mask,
                                 param.spec.default_val);
    p.param_.handle = param.handle;
}

// Group start and end - Note: no GroupBeginBuilder/GroupEndBuilder in current implementation
// Groups are created directly via groupStart()/groupEnd() helper functions

} // namespace detail

// Implementation of ParamSet::buildOne for AE - uses detail::addParam overload resolution
template<class Bag, class... Ps>
template<class Builder, class P>
void ParamSet<Bag, Ps...>::buildOne(Builder& builder, P& param) {
    detail::addParam(builder, param);
}

// Implementation of ParamSet::fetchOne for AE - accesses builder's internal param_
template<class Bag, class... Ps>
template<class Source, class P>
int ParamSet<Bag, Ps...>::fetchOne(const Source& source, Bag& bag, const P& p) const {
    // Access the internal param_ member from builder
    const auto& param = p.param_;

    // Skip parameters without values (like buttons and groups)
    if constexpr (std::is_same_v<typename P::val_type, std::nullptr_t>) {
        return 0;
    }

    // Checkout parameter
    PF_ParamDef d{{0}};
    if (auto err = source.co(param.handle.id, &d)) {
        return err;
    }

    // Extract value based on spec type
    if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
        float v = d.u.fs_d.value;
        if (param.spec.is_percent) {
            v *= 0.01f;
        }
        bag.*(param.member) = v;
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
        bag.*(param.member) = static_cast<typename P::val_type>(d.u.sd.value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
        auto& c = bag.*(param.member);
        c[0] = d.u.cd.value.red   / static_cast<float>(PF_MAX_CHAN8);
        c[1] = d.u.cd.value.green / static_cast<float>(PF_MAX_CHAN8);
        c[2] = d.u.cd.value.blue  / static_cast<float>(PF_MAX_CHAN8);
        c[3] = d.u.cd.value.alpha / static_cast<float>(PF_MAX_CHAN8);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
        bag.*(param.member) = (d.u.bd.value != 0);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
        if (d.u.bd.value != 0) {
            bag.*(param.member) |= param.spec.flag_mask;
        } else {
            bag.*(param.member) &= ~param.spec.flag_mask;
        }
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
        bag.*(param.member) = static_cast<typename P::val_type>(d.u.pd.value);
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
        float degrees = static_cast<float>(d.u.ad.value) / 65536.0f;
        bag.*(param.member) = degrees * static_cast<float>(M_PI) / 180.0f;
    } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
        auto& pt = bag.*(param.member);
        pt[0] = static_cast<float>(d.u.td.x_value >> 16);
        pt[1] = static_cast<float>(d.u.td.y_value >> 16);
    }

    // Checkin parameter
    source.ci(&d);
    return 0;
}

} // namespace mp

#endif // BUILD_FOR_AE
