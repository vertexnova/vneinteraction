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
 *   - buildReferenceFrame     — forward/right frame from world-up
 *   - mouseToApiScreen        — top-left mouse coords → API-native screen
 *   - mouseToNDC              — top-left mouse coords → NDC [-1, 1]
 *   - mouseUnproject          — API-aware unproject from mouse coords
 *   - mouseToWorldRay         — API-aware world ray from mouse coords
 *   - worldUnderCursor        — world point at distance along view ray
 *   - worldUnderCursorOrtho   — cursor world position for orthographic camera
 *   - worldUnderCursorPersp   — cursor world position for perspective camera
 *
 * Internal header — not installed, not included from any public header.
 */

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/types.h>
#include <vertexnova/math/projection_utils.h>
#include <vertexnova/math/viewport.h>
#include <vertexnova/scene/camera/camera.h>
#include <vertexnova/scene/camera/orthographic_camera.h>
#include <vertexnova/scene/camera/perspective_camera.h>

namespace vne::interaction {

// -----------------------------------------------------------------------------
// buildReferenceFrame
// -----------------------------------------------------------------------------

/**
 * @brief Build reference forward and right vectors from an arbitrary world-up.
 * ref_right is the actual RIGHT vector (ref_fwd × world_up), not left.
 */
void buildReferenceFrame(const vne::math::Vec3f& world_up,
                         vne::math::Vec3f& ref_fwd,
                         vne::math::Vec3f& ref_right) noexcept;

// -----------------------------------------------------------------------------
// Mouse coordinate conversions
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
 * @brief Convert top-left-origin mouse coords to NDC [-1, 1].
 * API-independent; for manual frustum geometry (pan, arcball, zoom-to-cursor).
 */
[[nodiscard]] vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept;

// -----------------------------------------------------------------------------
// API-aware unproject and ray from mouse
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

// -----------------------------------------------------------------------------
// Cursor world position helpers (ortho / perspective)
// -----------------------------------------------------------------------------

/**
 * @brief World position under cursor for orthographic camera (target plane).
 */
[[nodiscard]] vne::math::Vec3f worldUnderCursorOrtho(const vne::scene::OrthographicCamera& ortho,
                                                     float ndc_x,
                                                     float ndc_y) noexcept;

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
