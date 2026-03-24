/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#pragma once

/**
 * @file behavior_utils.h
 * @brief Extra behavior helpers layered on behavior_math.h (worldUnderCursorOrtho /
 *        worldUnderCursorPersp for zoom-to-cursor).
 *
 * Shared buildReferenceFrame, mouseToNDC, and related math live in behavior_math.h
 * only — include that header directly when you need those symbols without ortho/persp
 * helpers.
 * Internal header — used by tests and behaviors as needed.
 */

#include "vertexnova/interaction/behavior_math.h"

#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/scene/camera/orthographic_camera.h>
#include <vertexnova/scene/camera/perspective_camera.h>

namespace vne::interaction {

namespace detail {
constexpr float kBehaviorEpsilon = 1e-6f;
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
