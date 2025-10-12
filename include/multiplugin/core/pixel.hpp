#pragma once

#include <cstdint>
#include <algorithm>

namespace mp {

/**
 * @brief 8-bit ARGB pixel (After Effects byte order)
 *
 * Note: AE uses ARGB order, not RGBA
 */
struct Pixel8 {
    uint8_t a, r, g, b;

    static constexpr float max_value = 255.0f;

    inline float a_normalized() const { return a / max_value; }
    inline float r_normalized() const { return r / max_value; }
    inline float g_normalized() const { return g / max_value; }
    inline float b_normalized() const { return b / max_value; }

    inline void set_normalized(float fa, float fr, float fg, float fb) {
        a = static_cast<uint8_t>(std::clamp(fa * max_value, 0.0f, max_value));
        r = static_cast<uint8_t>(std::clamp(fr * max_value, 0.0f, max_value));
        g = static_cast<uint8_t>(std::clamp(fg * max_value, 0.0f, max_value));
        b = static_cast<uint8_t>(std::clamp(fb * max_value, 0.0f, max_value));
    }
};

/**
 * @brief 16-bit ARGB pixel (After Effects byte order)
 *
 * Note: AE uses unusual max value of 32768 for 16-bit, not 65535
 */
struct Pixel16 {
    uint16_t a, r, g, b;

    // AE's unusual 16-bit max value
    static constexpr float max_value = 32768.0f;

    inline float a_normalized() const { return a / max_value; }
    inline float r_normalized() const { return r / max_value; }
    inline float g_normalized() const { return g / max_value; }
    inline float b_normalized() const { return b / max_value; }

    inline void set_normalized(float fa, float fr, float fg, float fb) {
        a = static_cast<uint16_t>(std::clamp(fa * max_value, 0.0f, max_value));
        r = static_cast<uint16_t>(std::clamp(fr * max_value, 0.0f, max_value));
        g = static_cast<uint16_t>(std::clamp(fg * max_value, 0.0f, max_value));
        b = static_cast<uint16_t>(std::clamp(fb * max_value, 0.0f, max_value));
    }
};

/**
 * @brief 32-bit float ARGB pixel (After Effects byte order)
 */
struct Pixel32 {
    float a, r, g, b;

    static constexpr float max_value = 1.0f;

    inline float a_normalized() const { return a; }
    inline float r_normalized() const { return r; }
    inline float g_normalized() const { return g; }
    inline float b_normalized() const { return b; }

    inline void set_normalized(float fa, float fr, float fg, float fb) {
        a = fa;
        r = fr;
        g = fg;
        b = fb;
    }
};

// OpenFX pixel structures (RGBA byte order, different from AE!)
/**
 * @brief 8-bit RGBA pixel (OpenFX byte order)
 */
struct OFXPixel8 {
    uint8_t r, g, b, a;  // RGBA order for OFX

    static constexpr float max_value = 255.0f;

    inline float a_normalized() const { return a / max_value; }
    inline float r_normalized() const { return r / max_value; }
    inline float g_normalized() const { return g / max_value; }
    inline float b_normalized() const { return b / max_value; }

    inline void set_normalized(float fr, float fg, float fb, float fa) {
        r = static_cast<uint8_t>(std::clamp(fr * max_value, 0.0f, max_value));
        g = static_cast<uint8_t>(std::clamp(fg * max_value, 0.0f, max_value));
        b = static_cast<uint8_t>(std::clamp(fb * max_value, 0.0f, max_value));
        a = static_cast<uint8_t>(std::clamp(fa * max_value, 0.0f, max_value));
    }
};

/**
 * @brief 16-bit RGBA pixel (OpenFX byte order, full range)
 */
struct OFXPixel16 {
    uint16_t r, g, b, a;  // RGBA order

    // OFX uses full 16-bit range, not AE's 32768
    static constexpr float max_value = 65535.0f;

    inline float a_normalized() const { return a / max_value; }
    inline float r_normalized() const { return r / max_value; }
    inline float g_normalized() const { return g / max_value; }
    inline float b_normalized() const { return b / max_value; }

    inline void set_normalized(float fr, float fg, float fb, float fa) {
        r = static_cast<uint16_t>(std::clamp(fr * max_value, 0.0f, max_value));
        g = static_cast<uint16_t>(std::clamp(fg * max_value, 0.0f, max_value));
        b = static_cast<uint16_t>(std::clamp(fb * max_value, 0.0f, max_value));
        a = static_cast<uint16_t>(std::clamp(fa * max_value, 0.0f, max_value));
    }
};

/**
 * @brief 32-bit float RGBA pixel (OpenFX byte order)
 */
struct OFXPixel32F {
    float r, g, b, a;  // RGBA order

    static constexpr float max_value = 1.0f;

    inline float a_normalized() const { return a; }
    inline float r_normalized() const { return r; }
    inline float g_normalized() const { return g; }
    inline float b_normalized() const { return b; }

    inline void set_normalized(float fr, float fg, float fb, float fa) {
        r = fr;
        g = fg;
        b = fb;
        a = fa;
    }
};

// Note: Half float (OFXPixel16F) would require external library like half_float::half
// For now, we can treat it as regular OFXPixel16 in simplified version

// Type aliases for clarity
using PixelU8 = Pixel8;
using PixelU16 = Pixel16;
using PixelF32 = Pixel32;

// Pixel format enums supporting both AE and OFX
enum class PixelFormat {
    // After Effects formats (ARGB byte order)
    ARGB_8,   // PF_PixelFormat_ARGB32
    ARGB_16,  // PF_PixelFormat_ARGB64 (max 32768)
    ARGB_32F, // PF_PixelFormat_ARGB128

    // OpenFX formats (RGBA byte order)
    RGBA_8,   // OFX eBitDepthUByte
    RGBA_16,  // OFX eBitDepthUShort (max 65535)
    RGBA_16F, // OFX eBitDepthHalf
    RGBA_32F  // OFX eBitDepthFloat
};

/**
 * @brief Get pixel size in bytes for a given format
 */
constexpr size_t getPixelSize(PixelFormat format) {
    switch (format) {
        case PixelFormat::ARGB_8:   return sizeof(Pixel8);
        case PixelFormat::ARGB_16:  return sizeof(Pixel16);
        case PixelFormat::ARGB_32F: return sizeof(Pixel32);
        case PixelFormat::RGBA_8:   return sizeof(OFXPixel8);
        case PixelFormat::RGBA_16:  return sizeof(OFXPixel16);
        case PixelFormat::RGBA_16F: return sizeof(OFXPixel16);  // Simplified: same size
        case PixelFormat::RGBA_32F: return sizeof(OFXPixel32F);
        default: return 0;
    }
}

/**
 * @brief Template helper to get pixel type from format
 */
template<PixelFormat F> struct PixelTypeFromFormat;
template<> struct PixelTypeFromFormat<PixelFormat::ARGB_8>   { using type = Pixel8; };
template<> struct PixelTypeFromFormat<PixelFormat::ARGB_16>  { using type = Pixel16; };
template<> struct PixelTypeFromFormat<PixelFormat::ARGB_32F> { using type = Pixel32; };

template<PixelFormat F>
using PixelType = typename PixelTypeFromFormat<F>::type;

/**
 * @brief Mix/interpolate between two values
 */
template<typename T>
inline T lerp(T a, T b, float t) {
    return static_cast<T>(a * (1.0f - t) + b * t);
}

/**
 * @brief Convert from AE pixel (ARGB) to OFX pixel (RGBA)
 */
template<typename AEPixel, typename OFXPixel>
inline void convertAEtoOFX(const AEPixel& ae, OFXPixel& ofx) {
    // Convert considering different byte order and max values
    float r_norm = ae.r_normalized();
    float g_norm = ae.g_normalized();
    float b_norm = ae.b_normalized();
    float a_norm = ae.a_normalized();

    ofx.set_normalized(r_norm, g_norm, b_norm, a_norm);
}

/**
 * @brief Convert from OFX pixel (RGBA) to AE pixel (ARGB)
 */
template<typename OFXPixel, typename AEPixel>
inline void convertOFXtoAE(const OFXPixel& ofx, AEPixel& ae) {
    // Convert considering different byte order and max values
    float r_norm = ofx.r_normalized();
    float g_norm = ofx.g_normalized();
    float b_norm = ofx.b_normalized();
    float a_norm = ofx.a_normalized();

    ae.set_normalized(a_norm, r_norm, g_norm, b_norm);
}

/**
 * @brief Premultiply alpha
 */
template<typename PixelT>
inline void premultiply(PixelT& pixel) {
    float a_norm = pixel.a_normalized();
    pixel.r = static_cast<decltype(pixel.r)>(pixel.r * a_norm);
    pixel.g = static_cast<decltype(pixel.g)>(pixel.g * a_norm);
    pixel.b = static_cast<decltype(pixel.b)>(pixel.b * a_norm);
}

/**
 * @brief Unpremultiply alpha
 */
template<typename PixelT>
inline void unpremultiply(PixelT& pixel) {
    float a_norm = pixel.a_normalized();
    if (a_norm > 0.0f) {
        float inv_a = 1.0f / a_norm;
        pixel.r = static_cast<decltype(pixel.r)>(
            std::min(pixel.r * inv_a, PixelT::max_value));
        pixel.g = static_cast<decltype(pixel.g)>(
            std::min(pixel.g * inv_a, PixelT::max_value));
        pixel.b = static_cast<decltype(pixel.b)>(
            std::min(pixel.b * inv_a, PixelT::max_value));
    }
}

} // namespace mp