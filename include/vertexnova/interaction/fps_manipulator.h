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
 * @brief FPS-style (first-person shooter) camera manipulator.
 *
 * Implements classic FPS-style camera control:
 * - Mouse look: Rotate view with mouse movement
 * - Keyboard movement: WASD for forward/back/strafe, Q/E for up/down
 * - Sprint/slow modifiers: Shift for acceleration, Ctrl for deceleration
 * - Scrolling: Zoom via dolly (move forward/back) or FOV adjustment
 *
 * The camera moves relative to its local axes (forward = where camera looks).
 * Maintains proper world up vector orientation to prevent rolling.
 *
 * Supports both perspective and orthographic cameras. Zoom behavior is configurable:
 * - eDollyToCoi: Dolly movement in units per second
 * - eSceneScale: Scale the scene relative to camera position
 * - eChangeFov: Adjust field of view (perspective only)
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, FreeCameraBase
 */
class VNE_INTERACTION_API FpsManipulator final : public FreeCameraBase {
   public:
    /** Construct FPS manipulator with default settings. */
    FpsManipulator() noexcept;
    /** Destroy FPS manipulator. */
    ~FpsManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    FpsManipulator(const FpsManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    FpsManipulator& operator=(const FpsManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    FpsManipulator(FpsManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    FpsManipulator& operator=(FpsManipulator&&) noexcept = default;

    /** Check if this manipulator supports perspective projection. */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    /** Check if this manipulator supports orthographic projection. */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /** Set the viewport dimensions for mouse sensitivity calculation. */
    void setViewportSize(float width_px, float height_px) noexcept override;

    /** Reset manipulator state and animation. */
    void resetState() noexcept override;
    /** Adjust camera to frame the given bounding box. */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    /** Get world units per pixel for screen-to-world conversions. */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /** Set the zoom interaction method. */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    /** Get the current zoom method. */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    /** Set the world-space up vector (default: +Y). */
    void setWorldUp(const vne::math::Vec3f& up) noexcept;
    /** Set the camera movement speed in units per second (default: 5.0). */
    void setMoveSpeed(float units_per_sec) noexcept { move_speed_ = std::max(0.0f, units_per_sec); }
    /** Get the movement speed. */
    [[nodiscard]] float getMoveSpeed() const noexcept { return move_speed_; }
    /** Set the mouse look sensitivity in degrees per pixel (default: 0.1). */
    void setMouseSensitivity(float deg_per_pixel) noexcept { mouse_sensitivity_ = std::max(0.0f, deg_per_pixel); }
    /** Get the mouse sensitivity. */
    [[nodiscard]] float getMouseSensitivity() const noexcept { return mouse_sensitivity_; }
    /** Set the zoom speed for dolly movement or scene scaling (default: 5.0). */
    void setZoomSpeed(float units_or_factor) noexcept { zoom_speed_ = std::max(0.01f, units_or_factor); }
    /** Set the FOV zoom speed for scroll/pinch (perspective only, default: 1.5). */
    void setFovZoomSpeed(float factor) noexcept { fov_zoom_speed_ = std::max(0.01f, factor); }
    /** Get the FOV zoom speed. */
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }
    /** Set the sprint speed multiplier (default: 2.0). */
    void setSprintMultiplier(float mult) noexcept { sprint_mult_ = std::max(1.0f, mult); }
    /** Get the sprint speed multiplier. */
    [[nodiscard]] float getSprintMultiplier() const noexcept { return sprint_mult_; }
    /** Set the slow/crouch speed multiplier (default: 0.5, clamped to [0, 1]). */
    void setSlowMultiplier(float mult) noexcept { slow_mult_ = std::clamp(mult, 0.0f, 1.0f); }
    /** Get the slow speed multiplier. */
    [[nodiscard]] float getSlowMultiplier() const noexcept { return slow_mult_; }

   private:
    /** Compute the world-space up vector based on configuration. */
    [[nodiscard]] vne::math::Vec3f upVector() const noexcept override;

    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};  //!< World-space up vector (typically +Y axis)
};

}  // namespace vne::interaction
