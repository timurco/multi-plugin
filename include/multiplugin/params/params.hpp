/**
 * @file params.hpp
 * @brief Main parameter system header for MultiPlugin
 *
 * Based on UParams.hpp architecture with fluent API enhancements.
 *
 * Example usage:
 *   struct MyParams {
 *       float mix = 100.0f;
 *       int seed = 42;
 *   };
 *
 *   inline static auto kParams = mp::makeSet<MyParams>(
 *       mp::floatSlider(0, &MyParams::mix, "Mix", 0, 100, 100.0f)
 *           .percent()
 *           .precision(2),
 *
 *       mp::intSlider(1, &MyParams::seed, "Seed", 1, 1000, 42),
 *
 *       mp::button<MyParams>(2, "Randomize", "Generate random")
 *           .onClick([](auto& ctx) {
 *               ctx.setValue(&MyParams::seed, rand());
 *           })
 *   );
 */

#pragma once

#include "specs.hpp"
#include "param.hpp"
#include "context.hpp"
#include "param_builder.hpp"
#include "helpers.hpp"
#include "param_set.hpp"
#include "sources.hpp"

// NOTE: Backend-specific implementations (ae_param_impl.hpp, ofx_param_impl.hpp)
// are NOT included here. They must be included in backend .cpp files AFTER
// including the host SDK headers (AE SDK or OFX headers).
//
// User code (plugins) should only include this header, not the backend implementations.

// Namespace alias for UParams-style code
namespace up = mp;
