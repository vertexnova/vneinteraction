/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#pragma once

/**
 * @file behavior_utils.h
 * @brief Shared behavior utilities: buildReferenceFrame (actual RIGHT), mouseToNDC,
 *        worldUnderCursorOrtho / worldUnderCursorPersp for zoom-to-cursor.
 * Internal header — used by orbit_arcball, free_look, camera_behavior_base.
 */

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/scene/camera/orthographic_camera.h>
#include <vertexnova/scene/camera/perspective_camera.h>

namespace vne::interaction {

namespace detail {
constexpr float kBehaviorEpsilon = 1e-6f;
}

/**
 * @brief Build reference forward and right vectors from an arbitrary world-up.
 * ref_right is the actual RIGHT vector (ref_fwd × world_up), not left.
 */
inline void buildReferenceFrame(const vne::math::Vec3f& world_up,
                                vne::math::Vec3f& ref_fwd,
                                vne::math::Vec3f& ref_right) noexcept {
    vne::math::Vec3f candidate =
        (std::abs(world_up.y()) > 0.9f) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : vne::math::Vec3f(0.0f, -1.0f, 0.0f);
    ref_fwd = (candidate - world_up * candidate.dot(world_up));
    const float fwd_len = ref_fwd.length();
    ref_fwd = (fwd_len < detail::kBehaviorEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = ref_fwd.cross(world_up);  // Actual RIGHT (was world_up.cross(ref_fwd) = left)
    const float right_len = ref_right.length();
    if (right_len > detail::kBehaviorEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}

/**
 * @brief Convert top-left-origin mouse coordinates to NDC [-1,1].
 * Used for manual frustum geometry (zoom-to-cursor, etc.); API-independent.
 */
[[nodiscard]] inline vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return vne::math::Vec2f(0.0f, 0.0f);
    }
    return vne::math::Vec2f((2.0f * mx / w) - 1.0f, 1.0f - (2.0f * my / h));
}

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
    front = (front_len < detail::kBehaviorEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);
    const vne::math::Vec3f up = ortho.getUp().normalized();
    vne::math::Vec3f r = front.cross(up);
    const float r_len = r.length();
    r = (r_len < detail::kBehaviorEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / r_len);
    return target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
}

/**
 * @brief World position under cursor at given distance along view (perspective).
 */
[[nodiscard]] inline vne::math::Vec3f worldUnderCursorPersp(const vne::scene::PerspectiveCamera& persp,
                                                            float viewport_width,
                                                            float viewport_height,
                                                            float orbit_dist,
                                                            float ndc_x,
                                                            float ndc_y,
                                                            const vne::math::Vec3f& front,
                                                            const vne::math::Vec3f& right,
                                                            const vne::math::Vec3f& up) noexcept {
    const float fov_y_rad = vne::math::degToRad(persp.getFieldOfView());
    const float half_h = orbit_dist * vne::math::tan(fov_y_rad * 0.5f);
    const float aspect = (viewport_height > 0.0f) ? (viewport_width / viewport_height) : 1.0f;
    const float half_w = half_h * aspect;
    return persp.getPosition() + front * orbit_dist + right * (ndc_x * half_w) + up * (ndc_y * half_h);
}

}  // namespace vne::interaction
