/**
 * @file invert.cpp
 * @brief Example plugin demonstrating MultiPlugin framework
 *
 * Simple color inversion effect that works in both AE and OFX
 */

#include <multiplugin/multiplugin.hpp>

// Parameters structure (will be enhanced with UParams later)
struct InvertParams {
    float mix = 100.0f;  // Mix percentage (0-100)
};

/**
 * @brief Invert effect implementation
 */
class InvertEffect : public mp::PluginBase {
private:
    // Global data shared across all instances
    struct GlobalData {
        bool initialized = false;
        int instance_count = 0;
    };

    // Instance-specific data
    struct InstanceData {
        InvertParams params;
        int render_count = 0;
    };

    GlobalData& global_ = mp::Global<GlobalData>::get();

public:
    InvertEffect() = default;

    /**
     * @brief Called once when plugin loads
     */
    void onGlobalSetup() override {
        global_.initialized = true;
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
        // For now, hardcode params (will use UParams later)
        InvertParams params;
        params.mix = 100.0f;

        float mix_factor = params.mix / 100.0f;

        // Get pixel format from input
        auto format = ctx.getInput().getFormat();

        // Process based on bit depth
        switch (format) {
            // After Effects formats
            case mp::PixelFormat::ARGB_8:
                processPixels<mp::Pixel8>(ctx, mix_factor);
                break;

            case mp::PixelFormat::ARGB_16:
                processPixels<mp::Pixel16>(ctx, mix_factor);
                break;

            case mp::PixelFormat::ARGB_32F:
                processPixels<mp::Pixel32>(ctx, mix_factor);
                break;

            // OpenFX formats
            case mp::PixelFormat::RGBA_8:
                processPixels<mp::OFXPixel8>(ctx, mix_factor);
                break;

            case mp::PixelFormat::RGBA_16:
            case mp::PixelFormat::RGBA_16F: // treat half float as 16-bit for now
                processPixels<mp::OFXPixel16>(ctx, mix_factor);
                break;

            case mp::PixelFormat::RGBA_32F:
                processPixels<mp::OFXPixel32F>(ctx, mix_factor);
                break;
        }
    }

    /**
     * @brief Plugin metadata
     */
    mp::PluginInfo getInfo() const override {
        return {
            .name = PLUGIN_NAME,
            .category = PLUGIN_CATEGORY,
            .description = PLUGIN_DESCRIPTION,
            .vendor = PLUGIN_VENDOR,
            .support_url = PLUGIN_SUPPORT_URL
        };
    }

private:
    /**
     * @brief Process pixels with specific type
     */
    template<typename PixelT>
    void processPixels(mp::RenderContext& ctx, float mix_factor) {
        // Use parallel processing from the framework
        ctx.process<PixelT>([mix_factor](int x, int y, PixelT* in, PixelT* out) {
            // Invert RGB channels
            float inv_r = PixelT::max_value - in->r;
            float inv_g = PixelT::max_value - in->g;
            float inv_b = PixelT::max_value - in->b;

            // Mix with original
            out->r = static_cast<decltype(out->r)>(mp::lerp(static_cast<float>(in->r), inv_r, mix_factor));
            out->g = static_cast<decltype(out->g)>(mp::lerp(static_cast<float>(in->g), inv_g, mix_factor));
            out->b = static_cast<decltype(out->b)>(mp::lerp(static_cast<float>(in->b), inv_b, mix_factor));

            // Preserve alpha
            out->a = in->a;

            return false; // success
        });
    }
};

// Register the plugin
MP_REGISTER_PLUGIN(InvertEffect)
