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

        // Use processAuto to automatically dispatch to correct pixel type
        ctx.processAuto([mix_factor](int x, int y, auto* in, auto* out) {
            using PixelT = std::remove_pointer_t<decltype(in)>;

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
