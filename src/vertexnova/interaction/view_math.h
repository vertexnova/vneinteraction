#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file view_math.h
 * @brief World-space camera math utilities for camera manipulators.
 *
 * Provides:
 *   - worldUnderCursorOrtho  — cursor world position for orthographic camera (inline)
 *   - mouseUnproject         — API-aware unproject from mouse coords
 *   - mouseToWorldRay        — API-aware world ray from mouse coords
 *   - worldUnderCursor       — world point at distance along view ray
 *   - worldUnderCursorPersp  — cursor world position for perspective camera
 *
 * Internal header — not installed, not included from any public header.
 */

#include "camera_math.h"

#include <vertexnova/scene/camera/camera.h>
#include <vertexnova/scene/camera/orthographic_camera.h>
#include <vertexnova/scene/camera/perspective_camera.h>

namespace vne::interaction {

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

    // Orthonormalize image-plane up against view axis so NDC offsets don't pick up forward/back slip.
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
// World-space camera math — defined in camera_math.cpp
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
