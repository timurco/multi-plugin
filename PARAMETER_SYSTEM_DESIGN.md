# MultiPlugin Parameter System Design

## Executive Summary
This document outlines the comprehensive design for integrating a unified parameter system into the MultiPlugin framework, based on the proven UParams.hpp architecture. The system will enable single-source parameter declarations that work seamlessly with both After Effects and OpenFX hosts while maintaining type safety and compile-time verification.

## Core Concept: Unified Parameter Declaration

### The Single Source of Truth Principle
Parameters are declared once using compile-time template metaprogramming and automatically adapt to each host's requirements:

```cpp
// Example parameter declaration for any plugin
struct MyEffectParams {
    float mix = 100.0f;
    float[4] color = {1.0f, 0.8f, 0.35f, 1.0f};
    int iterations = 10;
    bool enabled = true;
    uint32_t flags = 0;  // For bit flags
};

// Compile-time parameter set definition
constexpr auto kParams = mp::params<MyEffectParams>(
    mp::slider(&MyEffectParams::mix, "Mix", 0, 100).percent().precision(2),
    mp::color(&MyEffectParams::color, "Ink Color").defaultValue(1.0f, 0.8f, 0.35f, 1.0f),
    mp::intSlider(&MyEffectParams::iterations, "Iterations", 1, 100).defaultValue(10),
    mp::checkbox(&MyEffectParams::enabled, "Enabled").defaultValue(true),
    mp::button("Randomize").onClick([](auto& ctx) {
        ctx.randomize(&MyEffectParams::mix);
    })
);
```

## Architecture Overview

### 1. Parameter Types and Specifications

```cpp
namespace mp {

// Base parameter specification types
struct SpecFloat { float min, max, slider_min, slider_max, default_val; int precision; };
struct SpecInt { int min, max, slider_min, slider_max, default_val; };
struct SpecColor { float default_color[4]; };
struct SpecBool { bool default_val; };
struct SpecButton { const char* label; std::function<void(ParamContext&)> onClick; };
struct SpecPopup { const char* items; int default_index; };
struct SpecAngle { float default_val; };
struct SpecPoint2D { float default_x, default_y; };
struct SpecFlag { uint32_t mask; bool default_val; };  // For bit flags
struct SpecGroup { bool is_start; };

// Parameter wrapper with member pointer binding
template<class Bag, class Spec, class Handle, class Val>
struct Param {
    Val Bag::* member;  // Pointer to member in params struct
    Spec spec;          // Specification
    Handle handle;      // Host-specific handle
};

}
```

### 2. Dual Pipeline Architecture

The system implements two distinct pipelines for parameter management:

#### A. Parameter Change Pipeline (User Actions)
Handles direct user interactions with parameters:

```cpp
template<typename Source>
void handleParameterChange(int param_id, const Source& source) {
    // Button clicks
    if (param_id == BUTTON_RANDOMIZE) {
        // Execute button callback
        kParams.executeButton(param_id, source);
    }
    // Preset changes
    else if (param_id == PRESET_DROPDOWN) {
        auto preset = kParams.getValue<int>(source, &Params::preset);
        applyPreset(source, preset);
    }
    // Value changes with side effects
    else if (param_id == MASTER_ENABLED) {
        auto enabled = kParams.getValue<bool>(source, &Params::master_enabled);
        if (!enabled) {
            // Disable all dependent parameters
            kParams.setEnabled(source, PARAM_DETAIL, false);
            kParams.setEnabled(source, PARAM_ITERATIONS, false);
        }
    }
}
```

#### B. UI Update Pipeline (State Management)
Updates parameter UI states based on current values:

```cpp
template<typename Source>
int updateParamsUI(const Source& source) {
    MyEffectParams current;
    kParams.fetch(source, current);

    // Enable/disable dependent parameters
    kParams.setEnabled(source, PARAM_COLOR, !current.use_original_color);
    kParams.setVisible(source, PARAM_ADVANCED_GROUP, current.show_advanced);

    // Update dynamic ranges
    if (current.mode == MODE_PRECISE) {
        kParams.setRange(source, PARAM_DETAIL, 0.001f, 1.0f);
    } else {
        kParams.setRange(source, PARAM_DETAIL, 0.1f, 10.0f);
    }

    return 0;
}
```

### 3. Host-Specific Backends

#### After Effects Backend Integration

```cpp
// In ae_backend.cpp

static PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data,
                         PF_ParamDef* params[], PF_LayerDef* output) {
    // Build parameters from compile-time definition
    AEParamBuilder builder(in_data, out_data);
    g_plugin->getParams().build(builder);

    out_data->num_params = builder.getCount() + 1; // +1 for input layer
    return PF_Err_NONE;
}

static PF_Err UserChangedParam(PF_InData* in_data, PF_OutData* out_data,
                               PF_ParamDef* params[], PF_LayerDef* output,
                               PF_UserChangedParamExtra* extra) {
    // Route to plugin's parameter change handler
    AESource source(in_data, out_data, params);
    int disk_id = g_plugin->getParams().getDiskIdByParamIndex(extra->param_index);
    g_plugin->handleParameterChange(disk_id, source);
    return PF_Err_NONE;
}

static PF_Err UpdateParamsUI(PF_InData* in_data, PF_OutData* out_data,
                            PF_ParamDef* params[], PF_LayerDef* output) {
    // Update UI states
    AESource source(in_data, out_data, params);
    return g_plugin->updateParamsUI(source);
}
```

#### OpenFX Backend Integration

```cpp
// In ofx_backend.cpp

static OfxStatus describeInContextAction(OfxImageEffectHandle effect,
                                        OfxPropertySetHandle inArgs) {
    // Build parameters from compile-time definition
    OFXParamBuilder builder(effect, gParamSuite);
    g_plugin->getParams().build(builder);
    return kOfxStatOK;
}

static OfxStatus instanceChangedAction(OfxImageEffectHandle effect,
                                      OfxPropertySetHandle inArgs) {
    // Get changed parameter
    char* param_name = nullptr;
    gPropertySuite->propGetString(inArgs, kOfxPropName, 0, &param_name);

    // Route to plugin's parameter change handler
    OfxSource source(effect, gParamSuite);
    int disk_id = g_plugin->getParams().getDiskIdByUniqueName(param_name);
    g_plugin->handleParameterChange(disk_id, source);

    // Update UI states
    g_plugin->updateParamsUI(source);

    return kOfxStatOK;
}
```

### 4. Enhanced Plugin Base Class

```cpp
namespace mp {

template<typename ParamsType>
class ParameterizedPlugin : public PluginBase {
protected:
    using Params = ParamsType;

public:
    // Parameter definition (must be implemented by plugin)
    virtual constexpr auto getParamDefinition() const = 0;

    // Optional parameter change handlers
    virtual void onParameterChanged(int param_id, const Params& params) {}
    virtual void onButtonClicked(int button_id, ParamContext& ctx) {}
    virtual void onPresetApplied(int preset_id, Params& params) {}

    // UI state management
    virtual void updateParameterStates(ParamUIContext& ctx, const Params& params) {}

    // Get current parameter values
    template<typename Source>
    Params getParams(const Source& source) const {
        Params params;
        getParamDefinition().fetch(source, params);
        return params;
    }

    // Set parameter value
    template<typename T, typename Source>
    void setParamValue(const Source& source, T Params::* member, const T& value) {
        getParamDefinition().setValue(source, member, value);
    }
};

}
```

### 5. Resource Generation System

#### Automatic .r File Generation for After Effects

The build system will automatically generate .r resource files from parameter definitions:

```python
# generate_params_r.py
def generate_r_file(params_def, output_path):
    """Generate AE resource file from parameter definition"""
    r_content = []

    for param in params_def:
        if param.type == "float_slider":
            r_content.append(f"""
                ADD_FLOAT_SLIDER("{param.name}",
                    {param.min}, {param.max},
                    {param.slider_min}, {param.slider_max},
                    {param.default}, {param.precision},
                    {param.display_flags});
            """)
        elif param.type == "color":
            r_content.append(f"""
                ADD_COLOR("{param.name}",
                    {param.default_r}, {param.default_g},
                    {param.default_b}, {param.default_a});
            """)
        # ... other parameter types

    write_r_file(output_path, r_content)
```

### 6. Advanced Features

#### A. Preset System

```cpp
struct PresetData {
    const char* name;
    MyEffectParams values;
};

constexpr PresetData kPresets[] = {
    {"Subtle", {.mix = 50.0f, .iterations = 5, .enabled = true}},
    {"Intense", {.mix = 100.0f, .iterations = 20, .enabled = true}},
    {"Artistic", {.mix = 75.0f, .iterations = 15, .enabled = true}}
};

void applyPreset(int preset_index, const auto& source) {
    const auto& preset = kPresets[preset_index];
    kParams.setValues(source, preset.values);
}
```

#### B. Dynamic Callbacks

```cpp
mp::button("Analyze").onClick([](ParamContext& ctx) {
    // Analyze input and adjust parameters
    auto analysis = ctx.analyzeInput();
    ctx.setValue(&Params::detail, analysis.recommended_detail);
    ctx.setValue(&Params::iterations, analysis.recommended_iterations);
    ctx.showMessage("Analysis complete!");
})
```

#### C. Conditional UI

```cpp
void updateParameterStates(ParamUIContext& ctx, const Params& params) {
    // Group visibility
    ctx.setGroupVisible("Advanced", params.show_advanced);

    // Dynamic enabling
    ctx.setEnabled(&Params::color, !params.use_original);

    // Conditional ranges
    if (params.precision_mode) {
        ctx.setRange(&Params::value, 0.001f, 1.0f, 0.001f);
    } else {
        ctx.setRange(&Params::value, 0.1f, 10.0f, 0.1f);
    }
}
```

## Implementation Plan

### Phase 1: Core Parameter System (Week 1-2)
1. ✅ Port parameter specification types from UParams.hpp
2. ✅ Implement compile-time parameter set builder
3. ✅ Create AESource and OfxSource abstractions
4. ✅ Implement fetch/setValue mechanisms

### Phase 2: Builder Pattern (Week 2-3)
1. ⬜ Create AEParamBuilder for After Effects
2. ⬜ Create OFXParamBuilder for OpenFX
3. ⬜ Implement parameter handle management
4. ⬜ Add group and custom UI support

### Phase 3: Backend Integration (Week 3-4)
1. ⬜ Integrate ParamsSetup in ae_backend.cpp
2. ⬜ Add PF_Cmd_USER_CHANGED_PARAM handling
3. ⬜ Add PF_Cmd_UPDATE_PARAMS_UI support
4. ⬜ Integrate OFX parameter actions

### Phase 4: UI State Management (Week 4-5)
1. ⬜ Implement setEnabled/setVisible for both hosts
2. ⬜ Add dynamic range adjustment
3. ⬜ Create updateParamsUI pipeline
4. ⬜ Test conditional UI scenarios

### Phase 5: Advanced Features (Week 5-6)
1. ⬜ Implement preset system
2. ⬜ Add callback system for buttons
3. ⬜ Create onClick/onChange handlers
4. ⬜ Add parameter validation

### Phase 6: Build System Integration (Week 6-7)
1. ⬜ Create CMake functions for parameter extraction
2. ⬜ Implement .r file generation for AE
3. ⬜ Add parameter metadata to plugin info
4. ⬜ Automate resource compilation

### Phase 7: Testing and Documentation (Week 7-8)
1. ⬜ Create comprehensive test plugin
2. ⬜ Test in After Effects CC 2020+
3. ⬜ Test in DaVinci Resolve
4. ⬜ Write developer documentation

## Key Benefits

1. **Single Declaration**: Parameters defined once, work everywhere
2. **Type Safety**: Compile-time verification of parameter types and bindings
3. **Zero Runtime Cost**: Template instantiation at compile time
4. **Host Agnostic**: Plugin code doesn't need to know about host specifics
5. **Modern C++**: Leverages C++17/20 features for clean API
6. **Automatic Resource Generation**: No manual .r file maintenance
7. **Powerful Callbacks**: onClick, onChange with full context access
8. **UI State Management**: Declarative UI updates based on parameter values
9. **Preset System**: Built-in support for parameter presets
10. **Extensible**: Easy to add new parameter types and features

## Migration Path for Existing Plugins

For plugins already using UParams.hpp:
1. Change namespace from `up::` to `mp::`
2. Update Source types to use mp::AESource/mp::OfxSource
3. Move parameter handling to plugin class methods
4. Update CMakeLists.txt to use mp_add_plugin

## Example: Complete Plugin with Parameters

```cpp
#include <multiplugin/multiplugin.hpp>

struct InvertParams {
    float mix = 100.0f;
    bool use_mask = false;
    int blend_mode = 0;
    float[4] tint_color = {1.0f, 1.0f, 1.0f, 1.0f};
};

class InvertEffect : public mp::ParameterizedPlugin<InvertParams> {
    static constexpr auto kParamDef = mp::params<InvertParams>(
        mp::slider(&InvertParams::mix, "Mix", 0, 100)
            .percent()
            .precision(2)
            .defaultValue(100.0f),

        mp::checkbox(&InvertParams::use_mask, "Use Mask")
            .defaultValue(false),

        mp::popup(&InvertParams::blend_mode, "Blend Mode",
                 "Normal|Screen|Multiply|Overlay")
            .defaultValue(0),

        mp::color(&InvertParams::tint_color, "Tint")
            .defaultValue(1.0f, 1.0f, 1.0f, 1.0f),

        mp::button("Reset")
            .onClick([](auto& ctx) {
                ctx.resetToDefaults();
            })
    );

public:
    constexpr auto getParamDefinition() const override {
        return kParamDef;
    }

    void onRender(RenderContext& ctx) override {
        auto params = getParams(ctx.getParamSource());

        ctx.processAuto([&](int x, int y, auto* in, auto* out) {
            float mix = params.mix / 100.0f;

            // Apply invert with mix
            out->r = mp::lerp(in->r, 1.0f - in->r, mix);
            out->g = mp::lerp(in->g, 1.0f - in->g, mix);
            out->b = mp::lerp(in->b, 1.0f - in->b, mix);

            // Apply tint if needed
            if (params.blend_mode > 0) {
                applyBlendMode(out, params.tint_color, params.blend_mode);
            }

            out->a = in->a;
        });
    }

    void updateParameterStates(ParamUIContext& ctx, const Params& params) override {
        // Tint color only available when blend mode is not Normal
        ctx.setEnabled(&InvertParams::tint_color, params.blend_mode > 0);
    }

    void onParameterChanged(int param_id, const Params& params) override {
        if (param_id == PARAM_BLEND_MODE) {
            LOG_DEBUG << "Blend mode changed to: " << params.blend_mode;
        }
    }
};

MP_REGISTER_PLUGIN(InvertEffect)
```

## Conclusion

This parameter system design provides a robust, type-safe, and maintainable solution for MultiPlugin's parameter needs. By leveraging compile-time metaprogramming and modern C++ features, we achieve zero-cost abstractions while maintaining a clean, declarative API that works seamlessly across both After Effects and OpenFX hosts.

The dual pipeline approach (handleParameterChange vs updateParamsUI) cleanly separates concerns and matches the event-driven nature of both host APIs. The system is extensible, allowing for future enhancements like GPU parameter buffers, network synchronization, and visual parameter editors.