#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file interaction_utils.h
 * @brief Internal NDC / screen / world-space helpers for camera manipulators.
 *
 * Merges the former @c camera_math.h and @c view_math.h. Not installed as public API.
 *
 * Provides:
 *   - buildReferenceFrame, mouseToNDC, mouseWindowToNDC, mouseWindowDeltaToNDCDelta
 *   - worldUnderCursorOrtho, mouseUnproject, mouseToWorldRay, worldUnderCursor,
 *     worldUnderCursorPersp
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
constexpr float kManipulatorUtilsEpsilon = 1e-6f;
/** |dot| above this ⇒ treat axis as nearly parallel to reference (pick alternate basis). */
constexpr float kNearlyParallelAxisDot = 0.9f;
/** Full NDC span when mapping window [0, size] → [-1, 1]. */
constexpr float kNdcFullSpan = 2.0f;
}  // namespace detail

// -----------------------------------------------------------------------------
// Reference frame + NDC (inline)
// -----------------------------------------------------------------------------

/**
 * @brief Build reference forward and right vectors from an arbitrary world-up.
 * ref_right is the actual RIGHT vector (ref_fwd × world_up), not left.
 */
inline void buildReferenceFrame(const vne::math::Vec3f& world_up,
                                vne::math::Vec3f& ref_fwd,
                                vne::math::Vec3f& ref_right) noexcept {
    vne::math::Vec3f up_n = world_up;
    const float up_len = up_n.length();
    up_n = (up_len < detail::kManipulatorUtilsEpsilon) ? vne::math::Vec3f(0.0f, 1.0f, 0.0f) : (up_n / up_len);

    vne::math::Vec3f candidate = (std::abs(up_n.y()) > detail::kNearlyParallelAxisDot)
                                     ? vne::math::Vec3f(0.0f, 0.0f, -1.0f)
                                     : vne::math::Vec3f(0.0f, -1.0f, 0.0f);
    ref_fwd = candidate - up_n * candidate.dot(up_n);
    const float fwd_len = ref_fwd.length();
    ref_fwd = (fwd_len < detail::kManipulatorUtilsEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = ref_fwd.cross(up_n);
    const float right_len = ref_right.length();
    if (right_len > detail::kManipulatorUtilsEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}

/**
 * @brief Convert top-left-origin mouse coords to NDC [-1, 1] (OpenGL-style Y only).
 * Prefer @ref mouseWindowToNDC with @c GraphicsApi for Vulkan/Metal/DX.
 */
[[nodiscard]] inline vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return {0.0f, 0.0f};
    }
    return {(detail::kNdcFullSpan * mx / w) - 1.0f, 1.0f - (detail::kNdcFullSpan * my / h)};
}

// -----------------------------------------------------------------------------
// API-dependent NDC — defined in interaction_utils.cpp
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

// -----------------------------------------------------------------------------
// worldUnderCursorOrtho — inline
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
    front = (front_len < detail::kManipulatorUtilsEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);

    vne::math::Vec3f up = ortho.getUp();
    up = up - front * front.dot(up);
    float up_len = up.length();
    if (up_len < detail::kManipulatorUtilsEpsilon) {
        const vne::math::Vec3f axes[3] = {vne::math::Vec3f(1.0f, 0.0f, 0.0f),
                                          vne::math::Vec3f(0.0f, 1.0f, 0.0f),
                                          vne::math::Vec3f(0.0f, 0.0f, 1.0f)};
        int best = 0;
        float best_abs_dot = std::abs(front.dot(axes[0]));
        for (int i = 1; i < 3; ++i) {
            const float ad = std::abs(front.dot(axes[i]));
            if (ad < best_abs_dot) {
                best_abs_dot = ad;
                best = i;
            }
        }
        up = axes[best] - front * front.dot(axes[best]);
        up_len = up.length();
    }
    if (up_len < detail::kManipulatorUtilsEpsilon) {
        vne::math::Vec3f alt(0.0f, 1.0f, 0.0f);
        if (std::abs(front.dot(alt)) > detail::kNearlyParallelAxisDot) {
            alt = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
        }
        up = front.cross(alt);
        up_len = up.length();
    }
    if (up_len < detail::kManipulatorUtilsEpsilon) {
        return target;
    }
    up = up / up_len;

    vne::math::Vec3f r = front.cross(up);
    const float r_len = r.length();
    r = (r_len < detail::kManipulatorUtilsEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / r_len);
    return target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
}

// -----------------------------------------------------------------------------
// World-space camera math — defined in interaction_utils.cpp
// -----------------------------------------------------------------------------

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
