/**
 * @file ofx_backend.cpp
 * @brief OpenFX backend implementation for MultiPlugin
 */

#include <ofxImageEffect.h>
#include <ofxMemory.h>
#include <ofxMultiThread.h>
#include <cstring>
#include <memory>
#include <mutex>
#include <cstdint>
#include <iostream>

#include "multiplugin/multiplugin.hpp"
#include "multiplugin/core/version.hpp"
#include "multiplugin/core/global.hpp"
#include "multiplugin/core/logger.hpp"

extern "C" mp::PluginBase* mp_create_plugin();
extern "C" void mp_destroy_plugin();

// Plugin instance data
static mp::PluginBase* g_plugin = nullptr;
static std::mutex g_plugin_mutex;

// OFX Suite pointers
static OfxHost* gHost = nullptr;
static const OfxPropertySuiteV1* gPropertySuite = nullptr;
static const OfxImageEffectSuiteV1* gEffectSuite = nullptr;
static const OfxParameterSuiteV1* gParamSuite = nullptr;

// Forward declarations
static OfxStatus pluginMain(const char* action, const void* handle,
                           OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);

// RenderContext implementation for OFX
namespace mp {

class OFXImageBuffer : public ImageBuffer {
private:
    OfxImageEffectHandle effect_;
    const OfxPropertySuiteV1* propSuite_;
    OfxPropertySetHandle imageProps_;
    void* data_;
    int width_;
    int height_;
    int rowBytes_;
    PixelFormat format_;

public:
    OFXImageBuffer(OfxImageEffectHandle effect,
                   const OfxPropertySuiteV1* propSuite,
                   OfxImageClipHandle clip,
                   OfxTime time)
        : effect_(effect), propSuite_(propSuite) {

        // Get image from clip
        OfxRectD region;
        gEffectSuite->clipGetRegionOfDefinition(clip, time, &region);
        gEffectSuite->clipGetImage(clip, time, &region, &imageProps_);

        // Get data pointer
        propSuite_->propGetPointer(imageProps_, kOfxImagePropData, 0, &data_);

        // Get dimensions
        OfxRectI bounds;
        propSuite_->propGetIntN(imageProps_, kOfxImagePropBounds, 4, &bounds.x1);
        width_ = bounds.x2 - bounds.x1;
        height_ = bounds.y2 - bounds.y1;

        // Get row bytes
        propSuite_->propGetInt(imageProps_, kOfxImagePropRowBytes, 0, &rowBytes_);

        // Determine pixel format from OFX properties
        char* components = nullptr;
        char* depth = nullptr;
        propSuite_->propGetString(imageProps_, kOfxImageEffectPropComponents, 0, &components);
        propSuite_->propGetString(imageProps_, kOfxImageEffectPropPixelDepth, 0, &depth);

        // Map OFX bit depth to our format (OFX uses RGBA byte order!)
        if (strcmp(depth, kOfxBitDepthByte) == 0) {
            format_ = PixelFormat::RGBA_8;
        } else if (strcmp(depth, kOfxBitDepthShort) == 0) {
            format_ = PixelFormat::RGBA_16;  // Full 16-bit range
        } else if (strcmp(depth, kOfxBitDepthHalf) == 0) {
            format_ = PixelFormat::RGBA_16F; // Half float
        } else if (strcmp(depth, kOfxBitDepthFloat) == 0) {
            format_ = PixelFormat::RGBA_32F;
        } else {
            format_ = PixelFormat::RGBA_8;  // Default
        }
    }

    ~OFXImageBuffer() {
        if (imageProps_) {
            gEffectSuite->clipReleaseImage(imageProps_);
        }
    }

    void* getData() override { return data_; }
    const void* getData() const override { return data_; }
    size_t getRowBytes() const override { return rowBytes_; }
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    PixelFormat getFormat() const override { return format_; }
};

class OFXRenderContext : public RenderContext {
private:
    OfxImageEffectHandle effect_;
    std::unique_ptr<OFXImageBuffer> input_;
    std::unique_ptr<OFXImageBuffer> output_;
    double time_;
    int frame_;

public:
    OFXRenderContext(OfxImageEffectHandle effect,
                    OfxPropertySetHandle renderArgs)
        : effect_(effect) {

        // Get time
        gPropertySuite->propGetDouble(renderArgs, kOfxPropTime, 0, &time_);

        // Calculate frame (assuming 24fps for now, should get from host)
        frame_ = static_cast<int>(time_ * 24.0);

        // Get input clip
        OfxImageClipHandle sourceClip = nullptr;
        gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName,
                                   &sourceClip, nullptr);

        // Get output clip
        OfxImageClipHandle outputClip = nullptr;
        gEffectSuite->clipGetHandle(effect, kOfxImageEffectOutputClipName,
                                   &outputClip, nullptr);

        // Create image buffers
        if (sourceClip) {
            input_ = std::make_unique<OFXImageBuffer>(effect, gPropertySuite, sourceClip, time_);
        }
        if (outputClip) {
            output_ = std::make_unique<OFXImageBuffer>(effect, gPropertySuite, outputClip, time_);
        }
    }

    ImageBuffer& getInput(int index = 0) override {
        return *input_;
    }

    ImageBuffer& getOutput() override {
        return *output_;
    }

    double getTime() const override { return time_; }
    int getFrame() const override { return frame_; }
};

} // namespace mp

// Action handlers
static OfxStatus renderAction(OfxImageEffectHandle effect,
                              OfxPropertySetHandle inArgs,
                              OfxPropertySetHandle outArgs)
{
    if (!g_plugin) {
        return kOfxStatFailed;
    }

    try {
        // Create render context
        mp::OFXRenderContext context(effect, inArgs);

        // Call plugin render
        g_plugin->onRender(context);

        return kOfxStatOK;
    }
    catch (...) {
        return kOfxStatFailed;
    }
}

static OfxStatus loadAction()
{
    // Create plugin instance
    std::lock_guard<std::mutex> lock(g_plugin_mutex);
    if (!g_plugin) {
        g_plugin = mp_create_plugin();
        if (g_plugin) {
            g_plugin->onGlobalSetup();
        } else {
            LOG_ERR << "Failed to create plugin instance!";
        }
    }
    return kOfxStatOK;
}

static OfxStatus unloadAction()
{
    std::lock_guard<std::mutex> lock(g_plugin_mutex);
    if (g_plugin) {
        g_plugin->onGlobalSetdown();
        mp_destroy_plugin();  // Properly destroys singleton and nulls the pointer
        g_plugin = nullptr;
    }
    return kOfxStatOK;
}

static OfxStatus describeAction(OfxImageEffectHandle effect)
{
    if (!g_plugin) {
        LOG_ERR << "Plugin instance not available in describe!";
        return kOfxStatFailed;
    }

    OfxPropertySetHandle props;
    gEffectSuite->getPropertySet(effect, &props);

    mp::PluginInfo info = g_plugin->getInfo();

    // Set plugin properties
    gPropertySuite->propSetString(props, kOfxPropLabel, 0, info.name);
    gPropertySuite->propSetString(props, kOfxPropShortLabel, 0, info.name);
    gPropertySuite->propSetString(props, kOfxPropLongLabel, 0, info.description);
    gPropertySuite->propSetString(props, kOfxPropPluginDescription, 0, info.description);
    gPropertySuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0,
                                  info.category);

    // Set supported contexts
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0,
                                  kOfxImageEffectContextFilter);
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 1,
                                  kOfxImageEffectContextGeneral);

    // Set supported pixel depths - OFX style with all bit depths
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0,
                                  kOfxBitDepthByte);      // 8-bit unsigned
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 1,
                                  kOfxBitDepthShort);     // 16-bit unsigned (full range)
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 2,
                                  kOfxBitDepthHalf);      // 16-bit half float
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 3,
                                  kOfxBitDepthFloat);     // 32-bit float

    // Set threading properties
    gPropertySuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, 0);
    gPropertySuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0,
                                 kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

static OfxStatus describeInContextAction(OfxImageEffectHandle effect,
                                        OfxPropertySetHandle inArgs)
{
    // Define clips
    OfxPropertySetHandle props;

    // Source clip
    gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0,
                                  kOfxImageComponentRGBA);
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 1,
                                  kOfxImageComponentAlpha);

    // Output clip
    gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0,
                                  kOfxImageComponentRGBA);
    gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 1,
                                  kOfxImageComponentAlpha);

    // Define parameters (will add UParams integration later)
    // For now, no parameters

    return kOfxStatOK;
}

static OfxStatus createInstanceAction(OfxImageEffectHandle effect)
{
    if (g_plugin) {
        g_plugin->onInstanceCreate();
    }
    return kOfxStatOK;
}

static OfxStatus destroyInstanceAction(OfxImageEffectHandle effect)
{
    if (g_plugin) {
        g_plugin->onInstanceDestroy();
    }
    return kOfxStatOK;
}

// Main plugin entry point
static OfxStatus pluginMain(const char* action, const void* handle,
                           OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    // Dispatch actions
    if (strcmp(action, kOfxActionLoad) == 0) {
        return loadAction();
    }
    else if (strcmp(action, kOfxActionUnload) == 0) {
        return unloadAction();
    }
    else if (strcmp(action, kOfxActionDescribe) == 0) {
        return describeAction(effect);
    }
    else if (strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContextAction(effect, inArgs);
    }
    else if (strcmp(action, kOfxActionCreateInstance) == 0) {
        return createInstanceAction(effect);
    }
    else if (strcmp(action, kOfxActionDestroyInstance) == 0) {
        return destroyInstanceAction(effect);
    }
    else if (strcmp(action, kOfxImageEffectActionRender) == 0) {
        return renderAction(effect, inArgs, outArgs);
    }

    return kOfxStatReplyDefault;
}

// Plugin factory
static void setHost(OfxHost* host)
{
    // Initialize logger with plugin-specific name
    mp::logger::set_log_base(std::string(PLUGIN_NAME) + "_OFX");

    gHost = host;
    if (host) {
        gPropertySuite = (const OfxPropertySuiteV1*)host->fetchSuite(
            host->host, kOfxPropertySuite, 1);
        gEffectSuite = (const OfxImageEffectSuiteV1*)host->fetchSuite(
            host->host, kOfxImageEffectSuite, 1);
        gParamSuite = (const OfxParameterSuiteV1*)host->fetchSuite(
            host->host, kOfxParameterSuite, 1);
        if (!gPropertySuite || !gEffectSuite || !gParamSuite) {
            LOG_ERR << "Failed to fetch OFX suites!";
        }
    } else {
        LOG_ERR << "Host is NULL!";
    }
}

// Plugin definition
static OfxPlugin pluginStruct = {
    kOfxImageEffectPluginApi,           // pluginApi
    1,                                  // apiVersion
    OFX_IDENTIFIER,                     // plugin unique ID from CMake
    mp::getPluginVersion().major,      // plugin major version
    mp::getPluginVersion().minor,      // plugin minor version
    setHost,                            // setHost function
    pluginMain                          // mainEntry function
};

// Entry points for OFX host
extern "C" {

/**
 * @brief Number of plugins in this binary
 */
int OfxGetNumberOfPlugins(void)
{
    return 1;
}

/**
 * @brief Get plugin at index
 */
OfxPlugin* OfxGetPlugin(int index)
{
    if (index == 0) {
        return &pluginStruct;
    }
    return nullptr;
}

const char* OfxGetAPIVersion(void)
{
    return kOfxImageEffectPluginApi;
}

} // extern "C"
