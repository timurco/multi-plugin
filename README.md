# MultiPlugin Framework

Universal C++ framework for building video effects plugins that work seamlessly with both After Effects (AE) and OpenFX (OFX) hosts.

## Features

- **Single Source**: Write plugin logic once, deploy to multiple hosts
- **Type Safety**: Strong C++ typing with compile-time verification
- **Modern C++17**: Clean, safe code with modern idioms
- **High Performance**: Platform-optimized parallel processing (GCD on macOS, parallel STL on Windows)
- **Minimal Recompilation**: Version changes only recompile one file
- **Automatic Resource Generation**: .r files generated from JSON configuration

## Quick Start

### Prerequisites

1. **Set environment variables**:
```bash
# macOS/Linux
export AE_SDK_PATH=/path/to/adobe-ae-sdk-2025
export OFX_PATH=/path/to/openfx

# Windows
set AE_SDK_PATH=C:\path\to\adobe-ae-sdk-2025
set OFX_PATH=C:\path\to\openfx
```

2. **Install CMake 3.20+**

### Building

```bash
mkdir build
cd build
cmake ..
make -j8
```

To build only for specific hosts:
```bash
cmake .. -DBUILD_AE_PLUGINS=ON -DBUILD_OFX_PLUGINS=OFF
```

## Writing a Plugin

```cpp
#include <multiplugin/multiplugin.hpp>

class MyEffect : public mp::PluginBase {
public:
    void onRender(mp::RenderContext& ctx) override {
        // Process pixels in parallel using your optimized system
        ctx.process<mp::Pixel8>([](int x, int y, auto* in, auto* out) {
            out->r = 255 - in->r;
            out->g = 255 - in->g;
            out->b = 255 - in->b;
            out->a = in->a;
            return false; // success
        });
    }

    mp::PluginInfo getInfo() const override {
        return {
            .name = "MyEffect",
            .category = "Color Correction",
            .description = "My awesome effect",
            .vendor = "MyCompany",
            .support_url = "https://example.com"
        };
    }
};

MP_REGISTER_PLUGIN(MyEffect)
```

## Architecture

### Core Components

- **Parallel Processing**: Platform-optimized threading (dispatch_apply on macOS, std::execution::par on Windows)
- **Pixel Formats**: Supports AE's ARGB byte order with proper 16-bit handling (max value 32768)
- **Global Singleton**: Thread-safe global data management
- **Version Isolation**: Version changes only trigger minimal recompilation

### Directory Structure

```
MultiPlugin/
├── include/multiplugin/    # Framework headers
│   ├── core/               # Core functionality
│   │   ├── parallel.hpp    # Your parallel processing system
│   │   ├── pixel.hpp       # Pixel format definitions
│   │   ├── global.hpp      # Global singleton
│   │   └── version.hpp     # Version management
│   └── multiplugin.hpp     # Main header
├── src/                    # Implementation
│   ├── core/              # Core implementations
│   └── backends/          # Host-specific code
│       ├── ae/           # After Effects backend
│       └── ofx/          # OpenFX backend
├── cmake/                 # CMake modules
├── tools/                # Build tools
│   ├── generate_pipl.py  # .r file generator
│   └── debug/            # Debugging helpers
│       └── resolve_simple.lldb
└── examples/             # Example plugins
    └── invert/          # Simple inversion effect
```

## Supported Hosts

### After Effects
- CC 2020 and later
- Supports 8-bit, 16-bit (with AE's unusual 32768 max), and 32-bit float

### OpenFX
- DaVinci Resolve 17+
- Nuke 13+
- Any OFX 1.4 compatible host

## Pixel Format Notes

The framework handles AE's unique pixel format quirks:
- **Byte Order**: ARGB (not RGBA)
- **16-bit Range**: 0-32768 (not 0-65535)
- **32-bit Float**: Standard 0.0-1.0 range

## Performance

The framework uses your optimized parallel processing system:
- **macOS**: Grand Central Dispatch (dispatch_apply)
- **Windows**: C++17 parallel algorithms (std::execution::par)
- **No PF_Iterate**: Avoids AE's slower iteration suites

## Integration with UParams

The framework is designed to work with your existing UParams.hpp universal parameter system. Simply include it and use as normal:

```cpp
#include "UParams.hpp"

struct MyParams {
    float mix = 100.0f;
    int seed = 42;
};

constexpr auto kParams = up::makeSet<MyParams>(
    up::floatSlider(&MyParams::mix, "Mix", 0, 100),
    up::intSlider(&MyParams::seed, "Seed", 0, 1000)
);
```

## Build & Install Targets

### Makefile shortcuts

At the repo root a convenience `Makefile` wraps the typical workflow. Edit the default `AE_SDK_PATH` / `OFX_PATH` inside it (or override on the command line).

```bash
# One-shot build with optimizations + debug symbols
make relwithdebinfo

# Strict release build
make release

# Debug build
make debug

# Install bundles into the host plug-in folders
make install

# Remove build tree
make distclean
```

All targets accept overrides, e.g. `make build CONFIG=Release AE_SDK_PATH=/custom/ae OFX_PATH=/custom/ofx`.

### Raw CMake (if you prefer)

- Configure once with your SDK paths:
  ```bash
  cmake -S . -B build \
    -DAE_SDK_PATH=/Users/you/adobe-ae-sdk-2025 \
    -DOFX_PATH=/Users/you/openfx \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
  ```
- Rebuild after changes:
  ```bash
  cmake --build build -j8
  ```
- Install compiled plugins:
  ```bash
  cmake --install build
  ```
  AE bundles copy to `/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/<PLUGIN_CATEGORY>`.
  OFX bundles copy to `/Library/OFX/Plugins/<PLUGIN_CATEGORY>`.
  On Windows the destinations are `C:\Program Files\Adobe\Common Plug-ins\7.0\MediaCore\<PLUGIN_CATEGORY>` and `C:\Program Files\Common Files\OFX\Plugins\<PLUGIN_CATEGORY>`.
  To stage under your home directory, append `--prefix ~` to the install command.

## Debugging OFX in Resolve

A minimal LLDB script lives at `tools/debug/resolve_simple.lldb`. Use it to launch Resolve with anti-debug bypass and useful breakpoints:

```bash
lldb -s tools/debug/resolve_simple.lldb -- \
  "/Applications/DaVinci Resolve/DaVinci Resolve.app/Contents/MacOS/Resolve"
```

When Resolve crashes, LLDB automatically prints a backtrace.

## License

Your license here

## Contributing

Contributions welcome! Please read CONTRIBUTING.md first.

## Acknowledgments

- Adobe After Effects SDK
- OpenFX Standard
- Your parallel processing implementation
