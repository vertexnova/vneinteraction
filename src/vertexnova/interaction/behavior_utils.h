/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#pragma once

/**
 * @file behavior_utils.h
 * @brief Internal geometry utilities for camera behaviors.
 *
 * Provides:
 *   - buildReferenceFrame     — forward/right frame from world-up (inline)
 *   - mouseToNDC              — top-left mouse coords → NDC [-1, 1] (inline)
 *   - mouseWindowToNDC        — top-left mouse + graphics API → NDC (behavior_utils.cpp)
 *   - mouseWindowDeltaToNDCDelta — pointer delta in window px → NDC delta (API-aware; pan / frustum math)
 *   - worldUnderCursorOrtho   — cursor world position for orthographic camera (inline)
 *   - mouseToApiScreen        — top-left mouse coords → API-native screen
 *   - mouseUnproject          — API-aware unproject from mouse coords
 *   - mouseToWorldRay         — API-aware world ray from mouse coords
 *   - worldUnderCursor        — world point at distance along view ray
 *   - worldUnderCursorPersp   — cursor world position for perspective camera
 *
 * Internal header — not installed, not included from any public header.
 * Inline functions are header-only (safe to call from test TUs without linking
 * vneinteraction). Non-inline functions are defined in behavior_utils.cpp and
 * compiled into the library.
 */

#include <cmath>

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/math/core/types.h>
#include <vertexnova/math/projection_utils.h>
#include <vertexnova/math/viewport.h>
#include <vertexnova/scene/camera/camera.h>
#include <vertexnova/scene/camera/orthographic_camera.h>
#include <vertexnova/scene/camera/perspective_camera.h>

namespace vne::interaction {

namespace detail {
constexpr float kBehaviorUtilsEpsilon = 1e-6f;
}

// -----------------------------------------------------------------------------
// buildReferenceFrame — inline (used by tests and behaviors)
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
    ref_fwd = (fwd_len < detail::kBehaviorUtilsEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = ref_fwd.cross(world_up);
    const float right_len = ref_right.length();
    if (right_len > detail::kBehaviorUtilsEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}

// -----------------------------------------------------------------------------
// mouseToNDC — inline (used by tests and behaviors)
// -----------------------------------------------------------------------------

/**
 * @brief Convert top-left-origin mouse coords to NDC [-1, 1] (OpenGL-style Y only).
 * Prefer @ref mouseWindowToNDC / @ref mouseWindowDeltaToNDCDelta with @c GraphicsApi for Vulkan/Metal/DX.
 */
[[nodiscard]] inline vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return vne::math::Vec2f(0.0f, 0.0f);
    }
    return vne::math::Vec2f((2.0f * mx / w) - 1.0f, 1.0f - (2.0f * my / h));
}

// -----------------------------------------------------------------------------
// worldUnderCursorOrtho — inline (used by tests and behaviors)
// -----------------------------------------------------------------------------

/**
 * @brief World position under cursor for orthographic camera (target plane).
 */
[[nodiscard]] inline vne::math::Vec3f worldUnderCursorOrtho(const vne::scene::OrthographicCamera& ortho,
                                                            float ndc_x,
                                                            float ndc_y) noexcept {
    const float half_w = ortho.getWidth() * 0.5f;
    const float half_h = ortho.getHeight() * 0.5f;
    const vne::math::Vec3f target = ortho.getTarget();
    vne::math::Vec3f front = target - ortho.getPosition();
    const float front_len = front.length();
    front = (front_len < detail::kBehaviorUtilsEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);
    const vne::math::Vec3f up = ortho.getUp().normalized();
    vne::math::Vec3f r = front.cross(up);
    const float r_len = r.length();
    r = (r_len < detail::kBehaviorUtilsEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / r_len);
    return target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
}

// -----------------------------------------------------------------------------
// Camera-API-dependent functions — defined in behavior_utils.cpp
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
 * Uses a viewport-center reference; result is invariant for affine window→NDC maps (typical full-viewport case).
 */
[[nodiscard]] vne::math::Vec2f mouseWindowDeltaToNDCDelta(
    float delta_x_px, float delta_y_px, float w, float h, vne::math::GraphicsApi api) noexcept;

/**
 * @brief API-aware unproject from mouse coords (mouse -> API screen -> unproject).
 */
[[nodiscard]] vne::math::Vec3f mouseUnproject(
    const vne::scene::ICamera& camera, float mx, float my, float depth, const vne::math::Viewport& vp) noexcept;

/**
 * @brief API-aware world ray from mouse coords.
 */
[[nodiscard]] vne::math::Ray mouseToWorldRay(const vne::scene::ICamera& camera,
                                             float mx,
                                             float my,
                                             const vne::math::Viewport& vp) noexcept;

/**
 * @brief World point under cursor at a given distance along view ray.
 */
[[nodiscard]] vne::math::Vec3f worldUnderCursor(
    const vne::scene::ICamera& camera, float mx, float my, float distance, const vne::math::Viewport& vp) noexcept;

/**
 * @brief World position under cursor at given distance along view (perspective).
 */
[[nodiscard]] vne::math::Vec3f worldUnderCursorPersp(const vne::scene::PerspectiveCamera& persp,
                                                     float viewport_width,
                                                     float viewport_height,
                                                     float orbit_dist,
                                                     float ndc_x,
                                                     float ndc_y,
                                                     const vne::math::Vec3f& front,
                                                     const vne::math::Vec3f& right,
                                                     const vne::math::Vec3f& up) noexcept;

}  // namespace vne::interaction
