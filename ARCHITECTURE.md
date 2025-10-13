# MultiPlugin Framework Architecture

## Overview
MultiPlugin is a universal C++ framework for building video effects plugins that work seamlessly with both After Effects (AE) and OpenFX (OFX) hosts.

## Core Design Principles

1. **Single Source of Truth**: Write plugin logic once, deploy to multiple hosts
2. **Type Safety**: Strong C++ typing with compile-time verification
3. **Modern C++**: Leverage C++17/20 features for cleaner, safer code
4. **Zero-Cost Abstractions**: Host-specific code paths resolved at compile time
5. **Declarative Parameters**: Define parameters once, use everywhere

## Architecture Layers

```
┌─────────────────────────────────────────────────┐
│             User Plugin Implementation          │
│         (MyEffect : public mp::PluginBase)      │
├─────────────────────────────────────────────────┤
│               MultiPlugin Core API              │
│   ┌─────────────────────────────────────────┐   │
│   │  Parameters  │  Rendering  │  Instance  │   │
│   │    System    │    System   │ Management │   │
│   └─────────────────────────────────────────┘   │
├─────────────────────────────────────────────────┤
│              Host Abstraction Layer             │
│   ┌────────────────┐    ┌────────────────┐      │
│   │   AE Backend   │    │  OFX Backend   │      │
│   └────────────────┘    └────────────────┘      │
├─────────────────────────────────────────────────┤
│              Build System (CMake)               │
│   ┌────────────────┐    ┌────────────────┐      │
│   │ Resource Gen   │    │  Plugin Bundle │      │
│   │   (.r files)   │    │   Assembly     │      │
│   └────────────────┘    └────────────────┘      │
└─────────────────────────────────────────────────┘
```

## Core Components

### 1. Plugin Base Class
```cpp
namespace mp {
    class PluginBase {
    public:
        // Lifecycle
        virtual void onGlobalSetup(GlobalContext& ctx) {}
        virtual void onInstanceCreate(InstanceContext& ctx) {}
        virtual void onInstanceDestroy(InstanceContext& ctx) {}

        // Rendering
        virtual void onRender(RenderContext& ctx) = 0;
        virtual void onPreRender(PreRenderContext& ctx) {}

        // Parameters
        virtual void onParameterChanged(ParamChangeContext& ctx) {}

        // Metadata
        virtual PluginInfo getInfo() const = 0;
    };
}
```

### 2. Parameter System
Enhanced version of UParams.hpp with:
- Automatic .r file generation for AE
- Runtime parameter introspection
- Preset system
- Undo/redo support

```cpp
struct MyParams {
    float mix = 100.0f;
    Color ink = {1.0f, 0.8f, 0.35f, 1.0f};
    int seed = 42;
    bool enabled = true;
};

constexpr auto kParams = mp::params<MyParams>(
    mp::slider(&MyParams::mix, "Mix", 0, 100).percent().precision(2),
    mp::color(&MyParams::ink, "Ink Color"),
    mp::intSlider(&MyParams::seed, "Seed", 1, 1000),
    mp::checkbox(&MyParams::enabled, "Enabled"),
    mp::button("Randomize").onClick([](auto& ctx) {
        ctx.setValue(&MyParams::seed, random());
    })
);
```

### 3. Render Context
Unified interface for pixel processing:

```cpp
class RenderContext {
public:
    // Input/Output
    ImageBuffer& getInput(int index = 0);
    ImageBuffer& getOutput();

    // Parameters
    template<class T>
    const T& getParams() const;

    // Time & Frame info
    double getTime() const;
    int getFrame() const;

    // Threading
    void processInParallel(PixelProcessor proc);

    // Host-specific features
    bool hasGPUSupport() const;
    void* getHostSpecificData();
};
```

### 4. Image Buffer Abstraction
Handle different pixel formats transparently:

```cpp
class ImageBuffer {
public:
    enum Format { RGBA8, RGBA16, RGBA32F };

    template<typename T>
    T* getPixel(int x, int y);

    template<typename Func>
    void forEach(Func f);

    int width() const;
    int height() const;
    Format format() const;
};
```

## Host-Specific Backends

### After Effects Backend
- Handles PF_Cmd dispatch
- Manages PF_Handle memory
- Generates .r files at build time
- Supports AEGP suites for advanced features

### OpenFX Backend
- Implements OFX::ImageEffect
- Uses OFX descriptor system
- RAII for resource management

## Build System

### CMake Configuration
```cmake
mp_add_plugin(
    NAME MyEffect
    SOURCES src/MyEffect.cpp

    # Metadata
    VERSION 1.0.0
    VENDOR "MyCompany"
    CATEGORY "Color Correction"
    DESCRIPTION "Advanced color effect"

    # Host support
    HOSTS AE OFX

    # AE-specific
    AE_MATCH_NAME "MC_MyEffect"
    AE_SUPPORT_URL "https://example.com"

    # OFX-specific
    OFX_IDENTIFIER "com.mycompany.myeffect"
)
```

### Resource Generation
The build system automatically:
1. Parses plugin metadata
2. Generates .r files for AE
3. Compiles resources with PiPLtool
4. Bundles into proper plugin structure

## Instance Management

### Global vs Instance Data
```cpp
class MyEffect : public mp::PluginBase {
    // Global data (shared across all instances)
    struct GlobalData {
        bool gpuAvailable;
        LicenseInfo license;
    };

    // Instance data (per layer/clip)
    struct InstanceData {
        MyParams params;
        CachedResults cache;
        int frameCount;
    };

    mp::GlobalStorage<GlobalData> global;
    mp::InstanceStorage<InstanceData> instances;
};
```

## Platform Support

### Operating Systems
- macOS (Intel & ARM)
- Windows
- Linux (OFX only)

### Host Applications
- After Effects CC 2020+
- Premiere Pro CC 2020+
- DaVinci Resolve 17+
- Nuke 13+
- Any OFX-compatible host

## Example Plugin

```cpp
#include <multiplugin/multiplugin.hpp>

struct InvertParams {
    float mix = 100.0f;
};

class InvertEffect : public mp::PluginBase {
    static constexpr auto params = mp::params<InvertParams>(
        mp::slider(&InvertParams::mix, "Mix", 0, 100).percent()
    );

public:
    PluginInfo getInfo() const override {
        return {
            .name = "Invert",
            .category = "Color Correction",
            .version = {1, 0, 0}
        };
    }

    void onRender(RenderContext& ctx) override {
        auto& params = ctx.getParams<InvertParams>();
        auto& input = ctx.getInput();
        auto& output = ctx.getOutput();

        output.forEach([&](int x, int y, auto* pixel) {
            auto* src = input.getPixel<decltype(pixel)>(x, y);
            float mix = params.mix / 100.0f;

            pixel->r = mp::lerp(src->r, 1.0f - src->r, mix);
            pixel->g = mp::lerp(src->g, 1.0f - src->g, mix);
            pixel->b = mp::lerp(src->b, 1.0f - src->b, mix);
            pixel->a = src->a;
        });
    }
};

MP_REGISTER_PLUGIN(InvertEffect)
```

## Future Enhancements

1. **GPU Processing**: Metal/CUDA/OpenCL abstraction
2. **Scripting**: Python/Lua bindings for parameters
3. **Network Rendering**: Distributed processing support
4. **AI Integration**: ML model loading and inference
5. **Visual Editor**: Node-based effect composition"