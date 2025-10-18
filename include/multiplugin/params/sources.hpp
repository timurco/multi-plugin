/**
 * @file sources.hpp
 * @brief Parameter source abstractions for AE and OFX
 *
 * Architecture matches UParams.hpp for low-level compatibility
 */

#pragma once

#include "specs.hpp"

#ifdef BUILD_FOR_AE
#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "AEFX_SuiteHelper.h"
#include "AEGP_SuiteHandler.h"
#include "Param_Utils.h"

namespace mp {

/**
 * @brief 2D downsample ratio helper (matches UParams.hpp lines 225-235)
 */
struct Ratio2D {
    PF_RationalScale x;
    PF_RationalScale y;

    // Default constructor
    Ratio2D() : x{1, 1}, y{1, 1} {}

    // Convenient constructor from PF_InData (matches UParams.hpp)
    explicit Ratio2D(const PF_InData* in_data)
        : x(in_data->downsample_x), y(in_data->downsample_y) {}
};

/**
 * @brief Helper function for ratio scaling (matches UParams.hpp lines 238-241)
 */
template <typename T>
inline int64_t RatioScale(T value, const PF_RationalScale& ratio) {
    return static_cast<int64_t>(value) * static_cast<int64_t>(ratio.num) / static_cast<int64_t>(ratio.den);
}

/**
 * @brief Helper function for ratio unscaling (matches UParams.hpp lines 243-246)
 */
template <typename T>
inline int64_t RatioUnscale(T value, const PF_RationalScale& ratio) {
    return static_cast<int64_t>(value) * static_cast<int64_t>(ratio.den) / static_cast<int64_t>(ratio.num);
}

/**
 * @brief After Effects parameter source (matches UParams.hpp lines 790-914)
 * Provides unified access to AE parameter values
 */
class AESource {
public:
    PF_InData* in_data;
    AEGP_PluginID aegp_id;
    PF_ParamDef** params;
    Ratio2D sample_ratio;  // Store downsample ratio

    // Constructor with explicit downsample ratio (matches UParams.hpp line 797)
    explicit AESource(PF_InData* i)
        : in_data{i}, aegp_id(0), params(nullptr), sample_ratio{i} {}

    // Constructor for RenderContext::fetchParams compatibility (PF_InData*, PF_OutData*)
    AESource(PF_InData* i, PF_OutData* /* out_data - not used in UParams */)
        : in_data{i}, aegp_id(0), params(nullptr), sample_ratio{i} {}

    // Constructor with AEGP_PluginID for AE stream operations (matches UParams.hpp line 800)
    AESource(PF_InData* i, AEGP_PluginID id)
        : in_data{i}, aegp_id(id), params(nullptr), sample_ratio{i} {}

    // Constructor with params array for direct parameter access (matches UParams.hpp line 803)
    AESource(PF_InData* i, PF_ParamDef** ps)
        : in_data{i}, aegp_id(0), params(ps), sample_ratio{i} {}

    // Constructor with both AEGP_PluginID and params array (matches UParams.hpp line 806)
    AESource(PF_InData* i, PF_ParamDef** ps, AEGP_PluginID id)
        : in_data{i}, aegp_id(id), params(ps), sample_ratio{i} {}

    /**
     * @brief Checkout a parameter (co - matches UParams.hpp line 808)
     */
    int co(int id, PF_ParamDef* d) const {
        return PF_CHECKOUT_PARAM(in_data, id, in_data->current_time,
                                in_data->time_step, in_data->time_scale, d);
    }

    /**
     * @brief Checkin a parameter (ci - matches UParams.hpp line 809)
     */
    int ci(PF_ParamDef* d) const {
        return PF_CHECKIN_PARAM(in_data, d);
    }

    /**
     * @brief Legacy names for compatibility
     */
    PF_Err checkout(int param_index, PF_ParamDef* param_def) const {
        return co(param_index, param_def);
    }

    PF_Err checkin(PF_ParamDef* param_def) const {
        return ci(param_def);
    }

    /**
     * @brief Set parameter value via AEGP streams (matches UParams.hpp lines 812-857)
     * Only works when aegp_id is set and not in Premiere
     */
    template<typename T>
    int setValueViaStream(int param_index, const T& value) const {
        if (!aegp_id || in_data->appl_id == 'PrMr') return 1; // Not available or Premiere

        PF_Err err = PF_Err_NONE;
        AEGP_SuiteHandler suites(in_data->pica_basicP);

        AEGP_EffectRefH effectH = nullptr;
        AEGP_StreamRefH streamH = nullptr;
        AEGP_StreamValue streamVal{};  // zero-init

        err = suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(aegp_id, in_data->effect_ref, &effectH);
        if (err) return err;

        err = suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(aegp_id, effectH, param_index, &streamH);
        if (err) {
            if (effectH) suites.EffectSuite4()->AEGP_DisposeEffect(effectH);
            return err;
        }

        streamVal.streamH = streamH;

        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int>) {
            streamVal.val.one_d = value;
        } else if constexpr (std::is_same_v<T, float[4]>) {
            // Color parameter: convert from 0-1 to 0-255 range
            const auto& c = static_cast<const float(&)[4]>(value);
            streamVal.val.color.redF = c[0];
            streamVal.val.color.greenF = c[1];
            streamVal.val.color.blueF = c[2];
            streamVal.val.color.alphaF = c[3];
        } else if constexpr (std::is_same_v<T, float[2]>) {
            // 2D point parameter: set x and y
            const auto& pt = static_cast<const float(&)[2]>(value);
            streamVal.val.two_d.x = pt[0];
            streamVal.val.two_d.y = pt[1];
        }

        err = suites.StreamSuite2()->AEGP_SetStreamValue(aegp_id, streamH, &streamVal);

        suites.StreamSuite2()->AEGP_DisposeStreamValue(&streamVal);
        if (effectH) suites.EffectSuite4()->AEGP_DisposeEffect(effectH);
        if (streamH) suites.StreamSuite5()->AEGP_DisposeStream(streamH);

        return err;
    }

    /**
     * @brief Set parameter value directly via params array (matches UParams.hpp lines 868-913)
     * Uses spec_type logic for universal parameter handling
     */
    template<typename P, typename T>
    int setValueDirect(int param_index, const P& p, const T& value) const {
        if (!params) return 1; // No params array available
        PF_ParamDef* param = params[param_index];
        if (!param) return 1;

        // Universal logic based on spec_type
        if constexpr (std::is_same_v<typename P::spec_type, SpecFloat>) {
            float v = value;
            if (p.spec.display_flags & PF_ValueDisplayFlag_PERCENT)
                v *= 100.0f;
            param->u.fs_d.value = v;
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecColor>) {
            if constexpr (std::is_same_v<T, float[4]>) {
                const auto& c = static_cast<const float(&)[4]>(value);
                param->u.cd.value.red   = PF_MAX_CHAN8 * c[0];
                param->u.cd.value.green = PF_MAX_CHAN8 * c[1];
                param->u.cd.value.blue  = PF_MAX_CHAN8 * c[2];
                param->u.cd.value.alpha = PF_MAX_CHAN8 * c[3];
            }
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecBool>) {
            param->u.bd.value = value ? 1 : 0;
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecFlag>) {
            bool flag_value = value & p.spec.flag_mask;
            param->u.bd.value = flag_value ? 1 : 0;
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecInt>) {
            param->u.sd.value = static_cast<A_long>(value);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecPopup>) {
            param->u.pd.value = static_cast<A_long>(value + 1);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecAngle>) {
            float degrees = value * 180.0f / M_PI;
            param->u.ad.value = static_cast<PF_Fixed>(degrees * 65536.0f);
        } else if constexpr (std::is_same_v<typename P::spec_type, SpecPoint2D>) {
            if constexpr (std::is_same_v<T, float[2]>) {
                const auto& pt = static_cast<const float(&)[2]>(value);
                float x_scaled = static_cast<float>(RatioScale(static_cast<int64_t>(pt[0]), sample_ratio.x));
                float y_scaled = static_cast<float>(RatioScale(static_cast<int64_t>(pt[1]), sample_ratio.y));
                param->u.td.x_value = static_cast<int>(x_scaled * 65536.0f);
                param->u.td.y_value = static_cast<int>(y_scaled * 65536.0f);
            }
        } else {
            return PF_Err_UNRECOGNIZED_PARAM_TYPE;
        }
        param->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
        return PF_Err_NONE;
    }
};

} // namespace mp

#endif // BUILD_FOR_AE

#ifdef BUILD_FOR_OFX
#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxParam.h>

namespace mp {

/**
 * @brief OpenFX parameter source (pure C API)
 * Provides unified access to OFX parameter values
 */
class OfxSource {
public:
    OfxParamSetHandle param_set;
    const OfxParameterSuiteV1* param_suite;
    OfxTime time;

    OfxSource(OfxParamSetHandle pset, const OfxParameterSuiteV1* suite, OfxTime t = 0.0)
        : param_set(pset), param_suite(suite), time(t) {}

    /**
     * @brief Get double parameter value
     */
    int pullDouble(const std::string& name, double* value) const {
        if (!param_suite || !param_set) return 1;
        OfxParamHandle param = nullptr;
        if (param_suite->paramGetHandle(param_set, name.c_str(), &param, nullptr) != kOfxStatOK)
            return 1;
        if (param_suite->paramGetValue(param, value) != kOfxStatOK)
            return 1;
        return 0;
    }

    /**
     * @brief Get int parameter value
     */
    int pullInt(const std::string& name, int* value) const {
        if (!param_suite || !param_set) return 1;
        OfxParamHandle param = nullptr;
        if (param_suite->paramGetHandle(param_set, name.c_str(), &param, nullptr) != kOfxStatOK)
            return 1;
        if (param_suite->paramGetValue(param, value) != kOfxStatOK)
            return 1;
        return 0;
    }

    /**
     * @brief Get bool parameter value
     */
    int pullBool(const std::string& name, bool* value) const {
        if (!param_suite || !param_set) return 1;
        OfxParamHandle param = nullptr;
        if (param_suite->paramGetHandle(param_set, name.c_str(), &param, nullptr) != kOfxStatOK)
            return 1;
        int int_value = 0;
        if (param_suite->paramGetValue(param, &int_value) != kOfxStatOK)
            return 1;
        *value = (int_value != 0);
        return 0;
    }

    /**
     * @brief Get choice parameter value
     */
    int pullChoice(const std::string& name, int* value) const {
        if (!param_suite || !param_set) return 1;
        OfxParamHandle param = nullptr;
        if (param_suite->paramGetHandle(param_set, name.c_str(), &param, nullptr) != kOfxStatOK)
            return 1;
        if (param_suite->paramGetValue(param, value) != kOfxStatOK)
            return 1;
        return 0;
    }

    /**
     * @brief Get color parameter value
     */
    int pullColor(const std::string& name, double* r, double* g, double* b, double* a) const {
        if (!param_suite || !param_set) return 1;
        OfxParamHandle param = nullptr;
        if (param_suite->paramGetHandle(param_set, name.c_str(), &param, nullptr) != kOfxStatOK)
            return 1;
        if (param_suite->paramGetValueAtTime(param, time, r, g, b, a) != kOfxStatOK)
            return 1;
        return 0;
    }

    /**
     * @brief Get 2D point parameter value
     */
    int pullDouble2D(const std::string& name, double* x, double* y) const {
        if (!param_suite || !param_set) return 1;
        OfxParamHandle param = nullptr;
        if (param_suite->paramGetHandle(param_set, name.c_str(), &param, nullptr) != kOfxStatOK)
            return 1;
        if (param_suite->paramGetValueAtTime(param, time, x, y) != kOfxStatOK)
            return 1;
        return 0;
    }
};

} // namespace mp

#endif // BUILD_FOR_OFX
