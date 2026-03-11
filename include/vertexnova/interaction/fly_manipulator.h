#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/detail/free_camera_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

/**
 * @brief Fly-style (six-degree-of-freedom) camera manipulator.
 *
 * Implements unconstrained 6-DOF flight camera control removing world-up constraints:
 * - Mouse look: Rotate view in all three axes (pitch, yaw, roll)
 * - Keyboard movement: Forward/back and lateral motion relative to camera orientation
 * - Sprint/slow modifiers: Shift for acceleration, Ctrl for deceleration
 * - Scrolling: Zoom via dolly or FOV adjustment
 *
 * Unlike FPS (which constrains to world-up), Fly allows complete freedom including
 * barrel roll. Camera can orient in any direction without restrictions.
 *
 * Supports both perspective and orthographic cameras. Multiple zoom methods:
 * - eDollyToCoi: Dolly movement in units per second
 * - eSceneScale: Scale the scene relative to camera
 * - eChangeFov: Adjust field of view (perspective only)
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, FreeCameraBase
 */
class VNE_INTERACTION_API FlyManipulator final : public FreeCameraBase {
   public:
    /** Construct fly manipulator with default settings. */
    FlyManipulator() noexcept;
    /** Destroy fly manipulator. */
    ~FlyManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    FlyManipulator(const FlyManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    FlyManipulator& operator=(const FlyManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    FlyManipulator(FlyManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    FlyManipulator& operator=(FlyManipulator&&) noexcept = default;

    /**
     * @brief Check if this manipulator supports perspective projection.
     * @return true (fly supports perspective)
     */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }

    /**
     * @brief Check if this manipulator supports orthographic projection.
     * @return true (fly supports orthographic)
     */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /**
     * @brief Set the viewport dimensions for mouse sensitivity calculation.
     * @param width_px Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    void setViewportSize(float width_px, float height_px) noexcept override;

    /** Reset manipulator state and animation. */
    void resetState() noexcept override;

    /**
     * @brief Adjust camera to frame the given bounding box.
     * @param min_world Minimum corner of AABB in world space
     * @param max_world Maximum corner of AABB in world space
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;

    /**
     * @brief Get world units per pixel for screen-to-world conversions.
     * @return World-space distance per pixel at center of view
     */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /**
     * @brief Set the zoom interaction method.
     * @param method Zoom method (dolly, scene scale, or FOV)
     */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }

    /**
     * @brief Get the current zoom method.
     * @return Current zoom method
     */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /**
     * @brief Set the camera movement speed in units per second (default: 5.0).
     * @param units_per_sec Movement speed (clamped to >= 0)
     */
    void setMoveSpeed(float units_per_sec) noexcept { move_speed_ = std::max(0.0f, units_per_sec); }

    /**
     * @brief Get the movement speed.
     * @return Current movement speed in units per second
     */
    [[nodiscard]] float getMoveSpeed() const noexcept { return move_speed_; }

    /**
     * @brief Set the mouse look sensitivity in degrees per pixel (default: 0.1).
     * @param deg_per_pixel Sensitivity (clamped to >= 0)
     */
    void setMouseSensitivity(float deg_per_pixel) noexcept { mouse_sensitivity_ = std::max(0.0f, deg_per_pixel); }

    /**
     * @brief Get the mouse sensitivity.
     * @return Current mouse sensitivity in degrees per pixel
     */
    [[nodiscard]] float getMouseSensitivity() const noexcept { return mouse_sensitivity_; }

    /**
     * @brief Set the zoom speed for dolly movement or scene scaling (default: 5.0).
     * @param units_or_factor Zoom speed (clamped to >= 0.01)
     */
    void setZoomSpeed(float units_or_factor) noexcept { zoom_speed_ = std::max(0.01f, units_or_factor); }

    /**
     * @brief Set the FOV zoom speed for scroll/pinch (perspective only, default: 1.5).
     * @param factor FOV zoom speed (clamped to >= 0.01)
     */
    void setFovZoomSpeed(float factor) noexcept { fov_zoom_speed_ = std::max(0.01f, factor); }

    /**
     * @brief Get the FOV zoom speed.
     * @return Current FOV zoom speed
     */
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }

    /**
     * @brief Set the sprint speed multiplier (default: 2.0).
     * @param mult Multiplier (clamped to >= 1.0)
     */
    void setSprintMultiplier(float mult) noexcept { sprint_mult_ = std::max(1.0f, mult); }

    /**
     * @brief Get the sprint speed multiplier.
     * @return Current sprint multiplier
     */
    [[nodiscard]] float getSprintMultiplier() const noexcept { return sprint_mult_; }

    /**
     * @brief Set the slow/crouch speed multiplier (default: 0.5, clamped to [0, 1]).
     * @param mult Multiplier (clamped to [0, 1])
     */
    void setSlowMultiplier(float mult) noexcept { slow_mult_ = std::clamp(mult, 0.0f, 1.0f); }

    /**
     * @brief Get the slow speed multiplier.
     * @return Current slow multiplier
     */
    [[nodiscard]] float getSlowMultiplier() const noexcept { return slow_mult_; }

   private:
    /** Compute the local up axis based on orientation. */
    [[nodiscard]] vne::math::Vec3f upAxis() const noexcept;
    /** Compute the world-space up vector. */
    [[nodiscard]] vne::math::Vec3f upVector() const noexcept override;
};

}  // namespace vne::interaction
