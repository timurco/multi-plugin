/**
 * @file invert.cpp
 * @brief Example plugin demonstrating MultiPlugin framework with parameters
 *
 * Simple color inversion effect that works in both AE and OFX
 */

#include <multiplugin/multiplugin.hpp>
#include <multiplugin/core/logger.hpp>

// Parameter disk IDs for serialization
enum {
    PARAM_MIX = 0,
    PARAM_CHANNEL_GROUP,
    PARAM_INVERT_RED,
    PARAM_INVERT_GREEN,
    PARAM_INVERT_BLUE,
    PARAM_CHANNEL_GROUP_END,
    PARAM_BLEND_MODE,
    PARAM_COLOR_TINT,
    PARAM_ADVANCED_GROUP,
    PARAM_THRESHOLD,
    PARAM_ADVANCED_GROUP_END
};

// Parameters structure
struct InvertParams {
    float mix = 100.0f;       // Mix percentage (0-100)
    bool invert_red = true;   // Invert red channel
    bool invert_green = true; // Invert green channel
    bool invert_blue = true;  // Invert blue channel
    int blend_mode = 0;       // 0=Normal, 1=Screen, 2=Multiply
    float color_tint[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA tint
    float threshold = 0.0f;   // Threshold value (0-100)
};

/**
 * @brief Invert effect implementation with parameter system
 */
class InvertEffect : public mp::PluginWithParams<InvertEffect> {
public:
    // Parameter set definition with fluent API (must be public for CRTP access)
    inline static auto kParams = mp::makeSet<InvertParams>(
        // Float slider with fluent methods
        mp::floatSlider(PARAM_MIX, &InvertParams::mix, "Mix",
                        0.0f, 100.0f, 100.0f).sliderRange(0.0f, 100.0f).percent().precision(1),

        // Channel group
        mp::groupBegin(PARAM_CHANNEL_GROUP, "Channels"),
        mp::checkbox(PARAM_INVERT_RED, &InvertParams::invert_red, "Invert Red", true),
        mp::checkbox(PARAM_INVERT_GREEN, &InvertParams::invert_green, "Invert Green", true),
        mp::checkbox(PARAM_INVERT_BLUE, &InvertParams::invert_blue, "Invert Blue", true),
        mp::groupEnd(PARAM_CHANNEL_GROUP_END),

        // Popup for blend mode
        mp::popup(PARAM_BLEND_MODE, &InvertParams::blend_mode, "Blend Mode",
                  "Normal|Screen|Multiply", 0),

        // Color parameter
        mp::color(PARAM_COLOR_TINT, &InvertParams::color_tint, "Color Tint",
                  {1.0f, 1.0f, 1.0f, 1.0f}),

        // Advanced group
        mp::groupBegin(PARAM_ADVANCED_GROUP, "Advanced"),
        mp::floatSlider(PARAM_THRESHOLD, &InvertParams::threshold, "Threshold",
                        0.0f, 100.0f, 0.0f).sliderRange(0.0f, 100.0f).precision(2),
        mp::groupEnd(PARAM_ADVANCED_GROUP_END)
    );

private:
    // Global data shared across all instances
    struct GlobalData {
        bool initialized = false;
        int instance_count = 0;
    };

    GlobalData& global_ = mp::Global<GlobalData>::get();

public:
    InvertEffect() = default;

    /**
     * @brief Called once when plugin loads
     */
    void onGlobalSetup() override {
        global_.initialized = true;

        mp::logger::set_plugin_version(mp::getPluginVersionString());
        mp::logger::set_min_level(mp::logger::lvl::trace);
        mp::logger::set_folder("TiM");
        mp::logger::set_log_base(getInfo().name);
    }

    /**
     * @brief Called when effect instance is created
     */
    void onInstanceCreate() override {
        global_.instance_count++;
    }

    /**
     * @brief Called when effect instance is destroyed
     */
    void onInstanceDestroy() override {
        global_.instance_count--;
    }

    /**
     * @brief Main render function
     */
    void onRender(mp::RenderContext& ctx) override {
        // Fetch parameter values from host
        InvertParams params;
        ctx.fetchParams(kParams, params);
        
        LOG_TRACE << "Frame: " << ctx.getInput().getWidth() << "x" << ctx.getInput().getHeight()
                  << " [" << ctx.getFrame() << "(" << ctx.getTime() << " s.)]"
                  << ", format: " << mp::getPixelFormatString(ctx.getInput().getFormat());

        // Use processAuto to automatically dispatch to correct pixel type
        ctx.processAuto([&](int x, int y, auto* in, auto* out) {
            using PixelT = std::remove_pointer_t<decltype(in)>;

            // Apply threshold (if pixel brightness below threshold, skip inversion)
            float brightness = (in->r + in->g + in->b) / (3.0f * PixelT::max_value);
            if (brightness * 100.0f < params.threshold) {
                // Below threshold - copy input to output
                *out = *in;
                return true; // success
            }

            // Invert channels based on flags
            float inv_r = params.invert_red ? (PixelT::max_value - in->r) : in->r;
            float inv_g = params.invert_green ? (PixelT::max_value - in->g) : in->g;
            float inv_b = params.invert_blue ? (PixelT::max_value - in->b) : in->b;

            // Apply blend mode
            float result_r, result_g, result_b;
            switch (params.blend_mode) {
                case 1: // Screen
                    result_r = PixelT::max_value - (PixelT::max_value - in->r) * (PixelT::max_value - inv_r) / PixelT::max_value;
                    result_g = PixelT::max_value - (PixelT::max_value - in->g) * (PixelT::max_value - inv_g) / PixelT::max_value;
                    result_b = PixelT::max_value - (PixelT::max_value - in->b) * (PixelT::max_value - inv_b) / PixelT::max_value;
                    break;
                case 2: // Multiply
                    result_r = (in->r * inv_r) / PixelT::max_value;
                    result_g = (in->g * inv_g) / PixelT::max_value;
                    result_b = (in->b * inv_b) / PixelT::max_value;
                    break;
                default: // Normal
                    result_r = inv_r;
                    result_g = inv_g;
                    result_b = inv_b;
                    break;
            }

            // Apply color tint
            result_r *= params.color_tint[0];
            result_g *= params.color_tint[1];
            result_b *= params.color_tint[2];

            // Mix with original
            out->r = static_cast<decltype(out->r)>(mp::lerp(static_cast<float>(in->r), result_r, params.mix));
            out->g = static_cast<decltype(out->g)>(mp::lerp(static_cast<float>(in->g), result_g, params.mix));
            out->b = static_cast<decltype(out->b)>(mp::lerp(static_cast<float>(in->b), result_b, params.mix));

            // Preserve alpha (with tint alpha applied)
            out->a = static_cast<decltype(out->a)>(in->a * params.color_tint[3]);

            return true; // success
        });
    }
};

// Register the plugin
MP_REGISTER_PLUGIN(InvertEffect)
