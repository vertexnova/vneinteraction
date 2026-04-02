#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_math.h
 * @brief Coordinate conversion and NDC utilities for camera manipulators.
 *
 * Provides:
 *   - buildReferenceFrame        — forward/right frame from world-up (inline)
 *   - mouseToNDC                 — top-left mouse coords → NDC [-1, 1] (inline)
 *   - mouseToApiScreen           — top-left mouse coords → API-native screen
 *   - mouseWindowToNDC           — top-left mouse + graphics API → NDC
 *   - mouseWindowDeltaToNDCDelta — pointer delta in window px → NDC delta (API-aware)
 *
 * Internal header — not installed, not included from any public header.
 */

#include <cmath>

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/math/core/types.h>
#include <vertexnova/math/projection_utils.h>
#include <vertexnova/math/viewport.h>

namespace vne::interaction {

namespace detail {
constexpr float kManipulatorUtilsEpsilon = 1e-6f;
}

// -----------------------------------------------------------------------------
// buildReferenceFrame — inline
// -----------------------------------------------------------------------------

/**
 * @brief Build reference forward and right vectors from an arbitrary world-up.
 * ref_right is the actual RIGHT vector (ref_fwd × world_up), not left.
 */
inline void buildReferenceFrame(const vne::math::Vec3f& world_up,
                                vne::math::Vec3f& ref_fwd,
                                vne::math::Vec3f& ref_right) noexcept {
    vne::math::Vec3f candidate =
        (std::abs(world_up.y()) > 0.9f) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : vne::math::Vec3f(0.0f, -1.0f, 0.0f);
    ref_fwd = candidate - world_up * candidate.dot(world_up);
    const float fwd_len = ref_fwd.length();
    ref_fwd = (fwd_len < detail::kManipulatorUtilsEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = ref_fwd.cross(world_up);
    const float right_len = ref_right.length();
    if (right_len > detail::kManipulatorUtilsEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}

// -----------------------------------------------------------------------------
// mouseToNDC — inline
// -----------------------------------------------------------------------------

/**
 * @brief Convert top-left-origin mouse coords to NDC [-1, 1] (OpenGL-style Y only).
 * Prefer @ref mouseWindowToNDC with @c GraphicsApi for Vulkan/Metal/DX.
 */
[[nodiscard]] inline vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return vne::math::Vec2f(0.0f, 0.0f);
    }
    return vne::math::Vec2f((2.0f * mx / w) - 1.0f, 1.0f - (2.0f * my / h));
}

// -----------------------------------------------------------------------------
// API-dependent functions — defined in camera_math.cpp
// -----------------------------------------------------------------------------

/**
 * @brief Convert top-left-origin mouse coords to API-native screen coords.
 * OpenGL expects bottom-left: flip Y. Vulkan/Metal/DirectX/WebGPU: pass-through.
 */
[[nodiscard]] vne::math::Vec2f mouseToApiScreen(float mx,
                                                float my,
                                                const vne::math::Viewport& vp,
                                                vne::math::GraphicsApi api) noexcept;

/**
 * @brief Top-left window/client mouse coords → NDC [-1, 1] for the given graphics API.
 */
[[nodiscard]] vne::math::Vec2f mouseWindowToNDC(
    float mx, float my, float w, float h, vne::math::GraphicsApi api) noexcept;

/**
 * @brief Pointer delta in top-left window pixels → change in NDC (same convention as @ref mouseWindowToNDC).
 */
[[nodiscard]] vne::math::Vec2f mouseWindowDeltaToNDCDelta(
    float delta_x_px, float delta_y_px, float w, float h, vne::math::GraphicsApi api) noexcept;

}  // namespace vne::interaction
