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

// Parameter system
#include "multiplugin/params/params.hpp"

// Forward declarations
namespace mp {
    class PluginBase;
    class RenderContext;
    struct PluginInfo;
    #ifdef BUILD_FOR_AE
    class AEParamBuilder;
    #elif BUILD_FOR_OFX
    class OFXParamBuilder;
    class OFXRenderContext;
    #endif
}

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

    // Parameter building - default implementation does nothing (for plugins without parameters)
    #ifdef BUILD_FOR_AE
    virtual void buildParams(AEParamBuilder& builder) {}
    #elif BUILD_FOR_OFX
    virtual void buildParams(OFXParamBuilder& builder) {}
    #endif

    // Parameter handling - dual pipeline for parameter changes and UI updates

    /**
     * @brief Handle parameter changes (User Actions Pipeline)
     * Called when user changes a parameter value or clicks a button
     * @param disk_id The disk ID of the changed parameter
     * @param backend_handle Host-specific handle for parameter access
     *
     * Example implementation:
     * ```cpp
     * void handleParameterChange(int disk_id, void* backend_handle) override {
     *     #ifdef BUILD_FOR_AE
     *     auto* pair = static_cast<std::pair<PF_InData*, PF_OutData*>*>(backend_handle);
     *     mp::AESource source(pair->first, pair->second);
     *     #endif
     *
     *     if (disk_id == BUTTON_RESET) {
     *         // Handle button click
     *     }
     * }
     * ```
     */
    virtual void handleParameterChange(int disk_id, void* backend_handle) {}

    /**
     * @brief Update parameter UI states (UI Update Pipeline)
     * Called to update parameter visibility, enabled state, ranges, etc.
     * @param backend_handle Host-specific handle for parameter access
     * @return 0 on success, non-zero on error
     *
     * Example implementation:
     * ```cpp
     * int updateParamsUI(void* backend_handle) override {
     *     #ifdef BUILD_FOR_AE
     *     auto* pair = static_cast<std::pair<PF_InData*, PF_OutData*>*>(backend_handle);
     *     mp::AESource source(pair->first, pair->second);
     *
     *     // Fetch current parameter values
     *     MyParams params;
     *     getParams().fetch(source, params);
     *
     *     // Update UI based on current state
     *     getParams().setEnabled(source, PARAM_COLOR, !params.use_original);
     *     #endif
     *     return 0;
     * }
     * ```
     */
    virtual int updateParamsUI(void* backend_handle) { return 0; }

    // Metadata - default implementation uses CMake-defined macros
    virtual PluginInfo getInfo() const {
        return {
            .name = PLUGIN_NAME,
            .category = PLUGIN_CATEGORY,
            .description = PLUGIN_DESCRIPTION,
            .vendor = PLUGIN_VENDOR,
            .support_url = PLUGIN_SUPPORT_URL
        };
    }

protected:
    PluginBase(const PluginBase&) = delete;
    PluginBase& operator=(const PluginBase&) = delete;
};

/**
 * @brief CRTP helper class for plugins with parameters
 *
 * Automatically implements buildParams() and provides type-safe access to kParams.
 * This keeps plugin examples clean - no need to override parameter-related methods.
 *
 * Usage:
 * @code
 * class MyPlugin : public mp::PluginWithParams<MyPlugin> {
 * public:
 *     inline static auto kParams = mp::makeSet<MyParams>(...);
 *
 *     void onRender(mp::RenderContext& ctx) override {
 *         // Use kParams directly
 *     }
 * };
 * @endcode
 *
 * @tparam Derived The derived plugin class (CRTP pattern)
 */
template<typename Derived>
class PluginWithParams : public PluginBase {
public:
    /**
     * @brief Build parameters using the derived class's kParams
     */
    #ifdef BUILD_FOR_AE
    void buildParams(mp::AEParamBuilder& builder) override {
        Derived::kParams.build(builder);
    }
    #elif BUILD_FOR_OFX
    void buildParams(OFXParamBuilder& builder) override {
        Derived::kParams.build(builder);
    }
    #endif

    /**
     * @brief Get pointer to parameter set (for backend access)
     * @return Pointer to derived class's kParams
     */
    void* getParamSetPtr() {
        return &Derived::kParams;
    }

protected:
    /**
     * @brief Get reference to kParams for use in plugin code
     */
    auto& getParams() { return Derived::kParams; }
    const auto& getParams() const { return Derived::kParams; }
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

    // Parameter fetching - implemented by backends
    virtual void* getBackendHandle() const = 0;

#ifdef BUILD_FOR_OFX
    // OFX-specific methods for parameter access (implemented in ofx_backend.cpp)
    virtual void* getOfxParamSetHandle() const { return nullptr; }
    virtual const void* getOfxParamSuite() const { return nullptr; }
#endif

    /**
     * @brief Fetch parameter values from host
     * @tparam Bag Parameter struct type
     * @tparam Ps Parameter types in the set
     * @param paramSet The parameter set definition
     * @param bag Output parameter struct to fill
     * @return 0 on success, non-zero on error
     */
    template<typename Bag, typename... Ps>
    int fetchParams(const ParamSet<Bag, Ps...>& paramSet, Bag& bag) {
        void* handle = getBackendHandle();
        if (!handle) return 1;

#ifdef BUILD_FOR_OFX
        // Get OFX-specific handles via virtual methods
        auto* paramSetHandle = static_cast<OfxParamSetHandle>(getOfxParamSetHandle());
        auto* paramSuite = static_cast<const OfxParameterSuiteV1*>(getOfxParamSuite());
        if (!paramSetHandle || !paramSuite) return 1;

        // Create source and fetch parameters
        OfxSource source(paramSetHandle, paramSuite, getTime());
        return paramSet.fetch(source, bag);
#elif defined(BUILD_FOR_AE)
        auto* pair = static_cast<std::pair<PF_InData*, PF_OutData*>*>(handle);
        AESource source(pair->first, pair->second);
        return paramSet.fetch(source, bag);
#else
        return 1;
#endif
    }

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

    /**
     * @brief Automatically dispatch to correct pixel type based on format
     *
     * Eliminates the need for switch statements in plugin code.
     * The lambda will be called with the correct pixel type.
     *
     * @param pixelFunc Lambda taking (int x, int y, PixelT* in, PixelT* out)
     * @return true on success
     */
    template<typename Func>
    bool processAuto(Func&& pixelFunc) {
        auto format = getInput().getFormat();

        switch (format) {
            // After Effects formats
            case PixelFormat::ARGB_8:
                return process<Pixel8>(std::forward<Func>(pixelFunc));

            case PixelFormat::ARGB_16:
                return process<Pixel16>(std::forward<Func>(pixelFunc));

            case PixelFormat::ARGB_32F:
                return process<Pixel32>(std::forward<Func>(pixelFunc));

            // OpenFX formats
            case PixelFormat::RGBA_8:
                return process<OFXPixel8>(std::forward<Func>(pixelFunc));

            case PixelFormat::RGBA_16:
                return process<OFXPixel16>(std::forward<Func>(pixelFunc));

            case PixelFormat::RGBA_16F:
                return process<OFXPixel16>(std::forward<Func>(pixelFunc));

            case PixelFormat::RGBA_32F:
                return process<OFXPixel32F>(std::forward<Func>(pixelFunc));

            default:
                return false;
        }
    }
};

} // namespace mp

// Macro for plugin registration
#ifdef BUILD_FOR_AE
    // AE creates new instance each time (managed by backend)
    #define MP_REGISTER_PLUGIN(PluginClass) \
        extern "C" mp::PluginBase* mp_create_plugin() { \
            return static_cast<::mp::PluginBase*>(new PluginClass()); \
        }
#endif

#ifdef BUILD_FOR_OFX
    // OFX uses singleton pattern (one instance per plugin binary)
    #define MP_REGISTER_PLUGIN(PluginClass) \
        static PluginClass* g_plugin_instance = nullptr; \
        extern "C" mp::PluginBase* mp_create_plugin() { \
            if (!g_plugin_instance) { \
                g_plugin_instance = new PluginClass(); \
            } \
            return g_plugin_instance; \
        } \
        extern "C" void mp_destroy_plugin() { \
            delete g_plugin_instance; \
            g_plugin_instance = nullptr; \
        }
#endif

#ifdef BUILD_FOR_AE
  #include "multiplugin/params/ae_param_impl.hpp"
  #elif BUILD_FOR_OFX
  #include "multiplugin/params/ofx_param_impl.hpp"
  #endif