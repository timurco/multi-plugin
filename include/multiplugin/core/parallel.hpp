#pragma once

#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>

#if defined(_WIN32)
#include <execution>
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#else
#include <thread>
#endif


#include "multiplugin/core/logger.hpp"

namespace mp {

/**
 * @brief Parallel pixel processor using platform-specific threading
 *
 * On Windows: Uses std::execution::par with parallel_for_each
 * On macOS: Uses Grand Central Dispatch (dispatch_apply)
 * On Linux: Uses std::thread pool with hardware_concurrency
 *
 * @tparam InPixel Source pixel type
 * @tparam OutPixel Destination pixel type
 */
template <typename InPixel, typename OutPixel>
class ParallelProcessor {
public:
    using PixelFunc = bool(*)(void*, int, int, InPixel*, OutPixel*);

    /**
     * @brief Iterate over an image in parallel, invoking pix_fn for every pixel
     *
     * @param pix_fn Function to call for each pixel
     * @param refcon User data passed to pixel function
     * @param srcPtr Source image data
     * @param srcPitch Bytes per row in source
     * @param dstPtr Destination image data
     * @param dstPitch Bytes per row in destination
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @return false on error, true on success
     */
    static bool iterate(
        PixelFunc pix_fn,
        void* refcon,
        const void* srcPtr, size_t srcPitch,
        void* dstPtr, size_t dstPitch,
        int width, int height)
    {
        if (!srcPtr || !dstPtr || !pix_fn || width <= 0 || height <= 0) {
            LOG_ERR << "ParallelProcessor: Invalid parameters";
            return false;
        }

        if (srcPitch < width * sizeof(InPixel) || dstPitch < width * sizeof(OutPixel)) {
            LOG_ERR << "ParallelProcessor: Invalid pitch";
            LOG_ERR << "Source Pitch [" << srcPitch << "] < width [" << width
                      << "] * sizeof(InPixel) [" << sizeof(InPixel) << "]";
            LOG_ERR << "Destination Pitch [" << dstPitch << "] < width [" << width
                      << "] * sizeof(OutPixel) [" << sizeof(OutPixel) << "]";
            return false;
        }

        const char* baseSrc = static_cast<const char*>(srcPtr);
        char* baseDst = static_cast<char*>(dstPtr);

        const size_t shiftIn = sizeof(InPixel);
        const size_t shiftOut = sizeof(OutPixel);

        auto rowIter = [&](int y) {
            const char* src = baseSrc + y * srcPitch;
            char* dst = baseDst + y * dstPitch;

            for (int x = 0; x < width; ++x) {
                pix_fn(refcon, x, y,
                       const_cast<InPixel*>(reinterpret_cast<const InPixel*>(src)),
                       reinterpret_cast<OutPixel*>(dst));
                src += shiftIn;
                dst += shiftOut;
            }
        };

        #if defined(_WIN32)
        // Windows: parallel_for_each
        std::vector<int> rows(height);
        std::iota(rows.begin(), rows.end(), 0);
        std::for_each(std::execution::par, rows.begin(), rows.end(), rowIter);
        #elif defined(__APPLE__)
        // macOS: Grand Central Dispatch
        dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_apply(height, q, ^(size_t y) {
            rowIter(static_cast<int>(y));
        });
        #else
        // Linux: std::thread pool
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // fallback

        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        for (unsigned int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t, numThreads]() {
                for (int y = t; y < height; y += numThreads) {
                    rowIter(y);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        #endif

        return true;
    }

    /**
     * @brief Lambda-friendly version of iterate
     *
     * @tparam Func Lambda or callable type
     * @param func Function object called as func(x, y, inPixel, outPixel)
     * @return true on success, false on error
     */
    template<typename Func>
    static bool iterateLambda(
        Func&& func,
        const void* srcPtr, size_t srcPitch,
        void* dstPtr, size_t dstPitch,
        int width, int height)
    {
        // Wrapper to convert lambda to function pointer
        auto wrapper = [](void* ctx, int x, int y, InPixel* in, OutPixel* out) -> bool {
            return (*static_cast<Func*>(ctx))(x, y, in, out);
        };

        return iterate(wrapper, &func, srcPtr, srcPitch, dstPtr, dstPitch, width, height);
    }

    /**
     * @brief Process rows in parallel with custom function
     *
     * @tparam Func Function type
     * @param func Function called as func(y_start, y_end) for row range
     * @param height Total height
     * @param tileHeight Rows per tile (optional, default=1)
     */
    template<typename Func>
    static void processRows(Func&& func, int height, int tileHeight = 1) {
        if (height <= 0 || tileHeight <= 0) return;

        int numTiles = (height + tileHeight - 1) / tileHeight;

        auto tileProcessor = [&](int tileIndex) {
            int yStart = tileIndex * tileHeight;
            int yEnd = std::min(yStart + tileHeight, height);
            func(yStart, yEnd);
        };

        #if defined(_WIN32)
        std::vector<int> tiles(numTiles);
        std::iota(tiles.begin(), tiles.end(), 0);
        std::for_each(std::execution::par, tiles.begin(), tiles.end(), tileProcessor);
        #elif defined(__APPLE__)
        dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_apply(numTiles, q, ^(size_t idx) {
            tileProcessor(static_cast<int>(idx));
        });
        #else
        // Linux: std::thread pool
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // fallback

        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        for (unsigned int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t, numThreads]() {
                for (int tile = t; tile < numTiles; tile += numThreads) {
                    tileProcessor(tile);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        #endif
    }
};

// Convenience aliases for common pixel types
template<typename T>
using SameTypeProcessor = ParallelProcessor<T, T>;

} // namespace mp
