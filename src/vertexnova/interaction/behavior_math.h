/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#pragma once

/**
 * @file behavior_math.h
 * @brief API-aware mouse-to-world utilities for multibackend (OpenGL, Vulkan,
 *        Metal, DirectX, WebGPU). Bridges top-left mouse input with
 *        projection_utils. Includes shared buildReferenceFrame.
 * Internal header — used by behaviors for zoom-to-cursor, picking, etc.
 */

#include <cmath>
#include <memory>

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/types.h>
#include <vertexnova/math/projection_utils.h>
#include <vertexnova/math/viewport.h>
#include <vertexnova/scene/camera/camera.h>

namespace vne::interaction {

namespace detail {
constexpr float kBehaviorMathEpsilon = 1e-6f;
}

// -----------------------------------------------------------------------------
// buildReferenceFrame (shared; ref_right is actual RIGHT)
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
    ref_fwd = (candidate - world_up * candidate.dot(world_up));
    const float fwd_len = ref_fwd.length();
    ref_fwd = (fwd_len < detail::kBehaviorMathEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = ref_fwd.cross(world_up);
    const float right_len = ref_right.length();
    if (right_len > detail::kBehaviorMathEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}

// -----------------------------------------------------------------------------
// Mouse to API-native screen (top-left mouse -> API screen coords)
// -----------------------------------------------------------------------------

/**
 * @brief Convert top-left-origin mouse coords to API-native screen coords.
 * OpenGL expects bottom-left: flip Y. Vulkan/Metal/DirectX/WebGPU: pass-through.
 */
[[nodiscard]] inline vne::math::Vec2f mouseToApiScreen(float mx,
                                                       float my,
                                                       const vne::math::Viewport& vp,
                                                       vne::math::GraphicsApi api) noexcept {
    if (!vne::math::screenOriginIsTopLeft(api)) {
        return vne::math::Vec2f(mx, vp.height - my);
    }
    return vne::math::Vec2f(mx, my);
}

/**
 * @brief Convert top-left-origin mouse coords to NDC [-1, 1].
 * API-independent; for manual frustum geometry (pan, arcball, zoom-to-cursor).
 */
[[nodiscard]] inline vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return vne::math::Vec2f(0.0f, 0.0f);
    }
    return vne::math::Vec2f((2.0f * mx / w) - 1.0f, 1.0f - (2.0f * my / h));
}

// -----------------------------------------------------------------------------
// API-aware unproject and ray from mouse
// -----------------------------------------------------------------------------

/**
 * @brief API-aware unproject from mouse coords (mouse -> API screen -> unproject).
 */
[[nodiscard]] inline vne::math::Vec3f mouseUnproject(
    const vne::scene::ICamera& camera, float mx, float my, float depth, const vne::math::Viewport& vp) noexcept {
    vne::math::GraphicsApi api = camera.getGraphicsApi();
    vne::math::Vec2f screen = mouseToApiScreen(mx, my, vp, api);
    vne::math::Mat4f inv_vp = camera.getViewProjectionMatrix().inverse();
    return vne::math::unproject(vne::math::Vec3f(screen.x(), screen.y(), depth), inv_vp, vp, api);
}

/**
 * @brief API-aware world ray from mouse coords.
 */
[[nodiscard]] inline vne::math::Ray mouseToWorldRay(const vne::scene::ICamera& camera,
                                                    float mx,
                                                    float my,
                                                    const vne::math::Viewport& vp) noexcept {
    vne::math::GraphicsApi api = camera.getGraphicsApi();
    vne::math::Vec2f screen = mouseToApiScreen(mx, my, vp, api);
    vne::math::Mat4f inv_vp = camera.getViewProjectionMatrix().inverse();
    return vne::math::screenToWorldRay(screen, inv_vp, vp, camera.getPosition(), api);
}

/**
 * @brief World point under cursor at a given distance along view ray.
 * For perspective: ray origin + direction * distance. Ortho: parallel projection
 * so same formula gives point at distance along view direction.
 */
[[nodiscard]] inline vne::math::Vec3f worldUnderCursor(
    const vne::scene::ICamera& camera, float mx, float my, float distance, const vne::math::Viewport& vp) noexcept {
    vne::math::Ray ray = mouseToWorldRay(camera, mx, my, vp);
    return ray.origin() + ray.direction() * distance;
}

/**
 * @brief Set camera pose from eye, look-at target, and up (ICamera has no lookAt).
 */
inline void setCameraLookAt(const std::shared_ptr<vne::scene::ICamera>& camera,
                            const vne::math::Vec3f& eye,
                            const vne::math::Vec3f& target,
                            const vne::math::Vec3f& up) noexcept {
    if (!camera) {
        return;
    }
    camera->setPosition(eye);
    camera->setTarget(target);
    camera->setUp(up);
    camera->updateMatrices();
}

}  // namespace vne::interaction
