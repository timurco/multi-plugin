#pragma once

/**
 * @file multiplugin.hpp
 * @brief Main header for MultiPlugin framework
 *
 * Universal plugin framework for After Effects and OpenFX
 */

// Core components
#include "multiplugin/core/version.hpp"
#include "multiplugin/core/pixel.hpp"
#include "multiplugin/core/parallel.hpp"
#include "multiplugin/core/global.hpp"

// Forward declarations
namespace mp {
    class PluginBase;
    class RenderContext;
    struct PluginInfo;
}

// Include your enhanced UParams if available
// #include "UParams.hpp"

// Platform detection
#if defined(BUILD_FOR_AE) && defined(BUILD_FOR_OFX)
    #error "Cannot build for both AE and OFX simultaneously. Choose one."
#endif

#if !defined(BUILD_FOR_AE) && !defined(BUILD_FOR_OFX)
    #warning "No host specified. Define BUILD_FOR_AE or BUILD_FOR_OFX"
#endif

namespace mp {

/**
 * @brief Plugin metadata
 */
struct PluginInfo {
    const char* name;
    const char* category;
    const char* description;
    const char* vendor;
    const char* support_url;
};

/**
 * @brief Base class for all plugins
 *
 * Derive from this class to create your plugin
 */
class PluginBase {
public:
    PluginBase() = default;
    virtual ~PluginBase() = default;

    // Lifecycle
    virtual void onGlobalSetup() {}
    virtual void onGlobalSetdown() {}
    virtual void onInstanceCreate() {}
    virtual void onInstanceDestroy() {}

    // Rendering
    virtual void onRender(RenderContext& ctx) = 0;

    // Metadata
    virtual PluginInfo getInfo() const = 0;

protected:
    PluginBase(const PluginBase&) = delete;
    PluginBase& operator=(const PluginBase&) = delete;
};

/**
 * @brief Image buffer abstraction
 */
class ImageBuffer {
public:
    virtual ~ImageBuffer() = default;

    virtual void* getData() = 0;
    virtual const void* getData() const = 0;
    virtual size_t getRowBytes() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual PixelFormat getFormat() const = 0;

    template<typename PixelT>
    PixelT* getPixel(int x, int y) {
        uint8_t* data = static_cast<uint8_t*>(getData());
        return reinterpret_cast<PixelT*>(data + y * getRowBytes() + x * sizeof(PixelT));
    }
};

/**
 * @brief Render context passed to plugin
 */
class RenderContext {
public:
    virtual ~RenderContext() = default;

    // Get input/output buffers
    virtual ImageBuffer& getInput(int index = 0) = 0;
    virtual ImageBuffer& getOutput() = 0;

    // Time information
    virtual double getTime() const = 0;
    virtual int getFrame() const = 0;

    // Process pixels in parallel
    template<typename InPixel, typename OutPixel, typename Func>
    bool processParallel(Func&& pixelFunc) {
        auto& input = getInput();
        auto& output = getOutput();

        return ParallelProcessor<InPixel, OutPixel>::iterateLambda(
            std::forward<Func>(pixelFunc),
            input.getData(), input.getRowBytes(),
            output.getData(), output.getRowBytes(),
            input.getWidth(), input.getHeight()
        );
    }

    // Process with same input/output type
    template<typename PixelType, typename Func>
    bool process(Func&& pixelFunc) {
        return processParallel<PixelType, PixelType>(std::forward<Func>(pixelFunc));
    }
};

} // namespace mp

// Macro for plugin registration
#ifdef BUILD_FOR_AE
    #define MP_REGISTER_PLUGIN(PluginClass) \
        extern "C" mp::PluginBase* mp_create_plugin() { \
            return new PluginClass(); \
        }
#endif

#ifdef BUILD_FOR_OFX
    #define MP_REGISTER_PLUGIN(PluginClass) \
        extern "C" mp::PluginBase* mp_create_plugin() { \
            return new PluginClass(); \
        }
#endif