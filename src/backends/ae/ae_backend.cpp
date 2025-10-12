/**
 * @file ae_backend.cpp
 * @brief After Effects backend implementation for MultiPlugin
 */

#include <AE_Effect.h>
#include <AE_EffectCB.h>
#include <AE_Macros.h>
#include <Param_Utils.h>
#include <AE_EffectCBSuites.h>
#include <String_Utils.h>
#include <AE_GeneralPlug.h>
#include <AEGP_SuiteHandler.h>
#include <AEFX_SuiteHelper.h>
#include <PrSDKPixelFormat.h>
#include <PrSDKAESupport.h>
#include <AE_PluginData.h>
#include <entry.h>

#include "multiplugin/multiplugin.hpp"
#include "multiplugin/core/version.hpp"
#include "multiplugin/core/global.hpp"

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "MultiPlugin"
#endif

extern "C" mp::PluginBase* mp_create_plugin();

#ifndef AE_MATCH_NAME
#define AE_MATCH_NAME "MP_MultiPlugin"
#endif

#ifndef PLUGIN_CATEGORY
#define PLUGIN_CATEGORY "MultiPlugin"
#endif

#ifndef PLUGIN_SUPPORT_URL
#define PLUGIN_SUPPORT_URL "https://github.com/multiplugin"
#endif

#ifndef AE_RESERVED_INFO
#define AE_RESERVED_INFO 0
#endif

#ifndef AE_INFO_FLAGS
#define AE_INFO_FLAGS 0
#endif

#ifndef AE_OUT_FLAGS
#define AE_OUT_FLAGS (PF_OutFlag_DEEP_COLOR_AWARE | \
                      PF_OutFlag_PIX_INDEPENDENT | \
                      PF_OutFlag_NON_PARAM_VARY)
#endif

#ifndef AE_OUT_FLAGS2
#define AE_OUT_FLAGS2 (PF_OutFlag2_FLOAT_COLOR_AWARE | \
                       PF_OutFlag2_SUPPORTS_SMART_RENDER | \
                       PF_OutFlag2_SUPPORTS_THREADED_RENDERING)
#endif
// Plugin-specific data
static mp::PluginBase* g_plugin = nullptr;

// Forward declarations
static PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data,
                          PF_ParamDef* params[], PF_LayerDef* output);
static PF_Err GlobalSetdown(PF_InData* in_data, PF_OutData* out_data,
                            PF_ParamDef* params[], PF_LayerDef* output);
static PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data,
                          PF_ParamDef* params[], PF_LayerDef* output);
static PF_Err About(PF_InData* in_data, PF_OutData* out_data,
                    PF_ParamDef* params[], PF_LayerDef* output);
static PF_Err Render(PF_InData* in_data, PF_OutData* out_data,
                     PF_ParamDef* params[], PF_LayerDef* output);
static PF_Err SequenceSetup(PF_InData* in_data, PF_OutData* out_data,
                            PF_ParamDef* params[], PF_LayerDef* output);
static PF_Err SequenceSetdown(PF_InData* in_data, PF_OutData* out_data,
                              PF_ParamDef* params[], PF_LayerDef* output);

// Helpers to get pixel format from world (simplified from ae_common.hpp)
static PF_Err GetPixelFormatFromWorld(PF_EffectWorld* world, PF_InData* in_data,
                                      PF_OutData* out_data, PF_PixelFormat& pixel_format)
{
    PF_Err err = PF_Err_NONE;

    AEGP_SuiteHandler suites(in_data->pica_basicP);
    AEFX_SuiteScoper<PF_WorldSuite2> world_suite(in_data, kPFWorldSuite,
                                                 kPFWorldSuiteVersion2, out_data);

    pixel_format = PF_PixelFormat_INVALID;
    ERR(world_suite->PF_GetPixelFormat(world, &pixel_format));

    // Get the Premiere pixel format suite if we're in Premiere
    if (in_data->appl_id == 'PrMr') {
        AEFX_SuiteScoper<PF_PixelFormatSuite> pixelFormatSuite(
            in_data, kPFPixelFormatSuite, kPFPixelFormatSuiteVersion, out_data);
        PrPixelFormat pr_pixel_format = PrPixelFormat_Invalid;
        ERR(pixelFormatSuite->GetPixelFormat(world, &pr_pixel_format));
        if (!err) {
            pixel_format = pr_pixel_format;
        }
    }

    return err;
}

// RenderContext implementation for AE
namespace mp {

class AEImageBuffer : public ImageBuffer {
private:
    PF_LayerDef* layer_;
    PixelFormat format_;

public:
    AEImageBuffer(PF_LayerDef* layer, PixelFormat format)
        : layer_(layer), format_(format) {}

    void* getData() override {
        return layer_->data;
    }

    const void* getData() const override {
        return layer_->data;
    }

    size_t getRowBytes() const override {
        return layer_->rowbytes;
    }

    int getWidth() const override {
        return layer_->width;
    }

    int getHeight() const override {
        return layer_->height;
    }

    PixelFormat getFormat() const override {
        return format_;
    }
};

class AERenderContext : public RenderContext {
private:
    PF_InData* in_data_;
    PF_OutData* out_data_;
    PF_LayerDef* input_;
    PF_LayerDef* output_;
    AEImageBuffer input_buffer_;
    AEImageBuffer output_buffer_;
    PixelFormat format_;

public:
    AERenderContext(PF_InData* in_data, PF_OutData* out_data,
                   PF_LayerDef* input, PF_LayerDef* output)
        : in_data_(in_data), out_data_(out_data),
          input_(input), output_(output),
          input_buffer_(input, determineFormat(output, in_data, out_data)),
          output_buffer_(output, determineFormat(output, in_data, out_data)),
          format_(determineFormat(output, in_data, out_data)) {}

    ImageBuffer& getInput(int index = 0) override {
        return input_buffer_;
    }

    ImageBuffer& getOutput() override {
        return output_buffer_;
    }

    double getTime() const override {
        // Convert from AE time format
        return static_cast<double>(in_data_->current_time) /
               static_cast<double>(in_data_->time_scale);
    }

    int getFrame() const override {
        // Calculate frame from time
        double fps = static_cast<double>(in_data_->time_scale) /
                    static_cast<double>(in_data_->time_step);
        return static_cast<int>(getTime() * fps);
    }

private:
    static PixelFormat determineFormat(PF_LayerDef* world, PF_InData* in_data,
                                      PF_OutData* out_data) {
        PF_PixelFormat pixel_format = PF_PixelFormat_INVALID;

        // Try to get pixel format from world
        PF_Err err = GetPixelFormatFromWorld(world, in_data, out_data, pixel_format);

        if (!err && pixel_format != PF_PixelFormat_INVALID) {
            // Map to our simplified format enum
            switch(pixel_format) {
                case PF_PixelFormat_ARGB32:
                    // PrPixelFormat_ARGB_4444_8u has same value as PF_PixelFormat_ARGB32
                    return PixelFormat::ARGB_8;

                case PF_PixelFormat_ARGB64:
                    return PixelFormat::ARGB_16;

                case PF_PixelFormat_ARGB128:
                    return PixelFormat::ARGB_32F;

                // Premiere-specific formats that differ from AE
                case PrPixelFormat_ARGB_4444_16u:
                    return PixelFormat::ARGB_16;

                case PrPixelFormat_ARGB_4444_32f:
                    return PixelFormat::ARGB_32F;

                default:
                    break;
            }
        }

        // Default to 8-bit if we can't determine
        return PixelFormat::ARGB_8;
    }
};

} // namespace mp

// Parameter enum
enum {
    INPUT_LAYER = 0,
    NUM_PARAMS
};

// GlobalSetup - called once when plugin loads
static PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data,
                          PF_ParamDef* params[], PF_LayerDef* output)
{
    PF_Err err = PF_Err_NONE;

    // Set version using the generated version function
    out_data->my_version = mp::getAEVersion();

    // Set plugin flags
    out_data->out_flags = AE_OUT_FLAGS;

    out_data->out_flags2 = AE_OUT_FLAGS2;

    // For Premiere Pro, specify supported pixel formats
    if (in_data->appl_id == 'PrMr') {
        auto pixelFormatSuite = AEFX_SuiteScoper<PF_PixelFormatSuite>(
            in_data, kPFPixelFormatSuite, kPFPixelFormatSuiteVersion, out_data);

        // Clear and add supported formats for Premiere
        (*pixelFormatSuite->ClearSupportedPixelFormats)(in_data->effect_ref);

        // Add ARGB formats only (no YUV)
        (*pixelFormatSuite->AddSupportedPixelFormat)(in_data->effect_ref,
            PrPixelFormat_ARGB_4444_8u);
        (*pixelFormatSuite->AddSupportedPixelFormat)(in_data->effect_ref,
            PrPixelFormat_ARGB_4444_16u);
        (*pixelFormatSuite->AddSupportedPixelFormat)(in_data->effect_ref,
            PrPixelFormat_ARGB_4444_32f);
    }

    // Create plugin instance
    if (!g_plugin) {
        g_plugin = mp_create_plugin();
        if (g_plugin) {
            g_plugin->onGlobalSetup();
        }
    }

    return err;
}

// GlobalSetdown - called once when plugin unloads
static PF_Err GlobalSetdown(PF_InData* in_data, PF_OutData* out_data,
                            PF_ParamDef* params[], PF_LayerDef* output)
{
    if (g_plugin) {
        g_plugin->onGlobalSetdown();
        delete g_plugin;
        g_plugin = nullptr;
    }
    return PF_Err_NONE;
}

// ParamsSetup - define parameters (none for now, will integrate UParams later)
static PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data,
                          PF_ParamDef* params[], PF_LayerDef* output)
{
    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    // For now, just set the number of params (input layer only)
    out_data->num_params = NUM_PARAMS;

    return err;
}

// About - show plugin info
static PF_Err About(PF_InData* in_data, PF_OutData* out_data,
                    PF_ParamDef* params[], PF_LayerDef* output)
{
    if (g_plugin) {
        mp::PluginInfo info = g_plugin->getInfo();

        // Create about string
        char about_str[256];
        snprintf(about_str, sizeof(about_str),
                "%s v%d.%d.%d\n%s\n\nby %s\n%s",
                info.name,
                mp::getPluginVersion().major,
                mp::getPluginVersion().minor,
                mp::getPluginVersion().patch,
                info.description,
                info.vendor,
                info.support_url);

        // Use suites to show dialog
        AEGP_SuiteHandler suites(in_data->pica_basicP);
        suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, about_str);
    }

    return PF_Err_NONE;
}

// SequenceSetup - called when effect is applied to layer
static PF_Err SequenceSetup(PF_InData* in_data, PF_OutData* out_data,
                            PF_ParamDef* params[], PF_LayerDef* output)
{
    if (g_plugin) {
        g_plugin->onInstanceCreate();
    }
    return PF_Err_NONE;
}

// SequenceSetdown - called when effect is removed from layer
static PF_Err SequenceSetdown(PF_InData* in_data, PF_OutData* out_data,
                              PF_ParamDef* params[], PF_LayerDef* output)
{
    if (g_plugin) {
        g_plugin->onInstanceDestroy();
    }
    return PF_Err_NONE;
}

// Render - main rendering function
static PF_Err Render(PF_InData* in_data, PF_OutData* out_data,
                     PF_ParamDef* params[], PF_LayerDef* output)
{
    PF_Err err = PF_Err_NONE;

    if (!g_plugin) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    // Get input layer
    PF_LayerDef* input = &params[INPUT_LAYER]->u.ld;

    // Copy input to output first (in case of errors)
    ERR(PF_COPY(input, output, NULL, NULL));

    if (!err) {
        // Create render context
        mp::AERenderContext context(in_data, out_data, input, output);

        // Call plugin render
        g_plugin->onRender(context);
    }

    return err;
}

// Main entry point
extern "C"
#ifdef _WIN32
DllExport
#endif
PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data,
                  PF_ParamDef* params[], PF_LayerDef* output, void* extra)
{
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;

            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;

            case PF_Cmd_GLOBAL_SETDOWN:
                err = GlobalSetdown(in_data, out_data, params, output);
                break;

            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;

            case PF_Cmd_SEQUENCE_SETUP:
                err = SequenceSetup(in_data, out_data, params, output);
                break;

            case PF_Cmd_SEQUENCE_SETDOWN:
                err = SequenceSetdown(in_data, out_data, params, output);
                break;

            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;

            default:
                // Ignore other commands
                break;
        }
    }
    catch (PF_Err& thrown_err) {
        err = thrown_err;
    }
    catch (...) {
        err = PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    return err;
}

// Modern entry point for CC 2020+
// This needs to use the exact same values as in the PiPL resource
extern "C"
#ifdef _WIN32
DllExport
#endif
PF_Err PluginDataEntryFunction2(
    PF_PluginDataPtr inPtr,
    PF_PluginDataCB2 inPluginDataCallBackPtr,
    SPBasicSuite* inSPBasicSuitePtr,
    const char* inHostName,
    const char* inHostVersion)
{
    PF_Err result = PF_Err_INVALID_CALLBACK;

    // Use macros from CMake that match what's in the PiPL
    result = PF_REGISTER_EFFECT_EXT2(
        inPtr,
        inPluginDataCallBackPtr,
        PLUGIN_NAME,                 // Name from CMake (same as PiPL)
        AE_MATCH_NAME,              // Match name from CMake (MUST match PiPL!)
        PLUGIN_CATEGORY,            // Category from CMake (same as PiPL)
        AE_RESERVED_INFO,           // Reserved info
        "EffectMain",               // Entry point (must match PiPL)
        PLUGIN_SUPPORT_URL);        // Support URL (optional but should match)

    return result;
}
