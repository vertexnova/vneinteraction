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

/**
 * @file ortho_pan_zoom_behavior.h
 * @brief OrthoPanZoomBehavior — orthographic 2D pan + zoom behavior.
 *
 * For orthographic viewports. Handles eBeginPan, ePanDelta, eEndPan,
 * eZoomAtCursor, eResetView. Inertia via exponential decay on pan velocity.
 */

#include "vertexnova/interaction/camera_behavior_base.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Orthographic 2D pan and zoom camera behavior.
 *
 * Does not handle rotation. Pan is in screen-to-world pixel coordinates.
 * Zoom-to-cursor preserves the world point under the cursor. Inertia is
 * applied to pan velocity via exponential decay.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API OrthoPanZoomBehavior final : public CameraBehaviorBase {
   public:
    OrthoPanZoomBehavior() noexcept = default;
    ~OrthoPanZoomBehavior() noexcept override = default;

    OrthoPanZoomBehavior(const OrthoPanZoomBehavior&) = delete;
    OrthoPanZoomBehavior& operator=(const OrthoPanZoomBehavior&) = delete;
    OrthoPanZoomBehavior(OrthoPanZoomBehavior&&) noexcept = default;
    OrthoPanZoomBehavior& operator=(OrthoPanZoomBehavior&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eBeginPan, ePanDelta, eEndPan, eZoomAtCursor, eResetView.
     */
    bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept override;

    /** Advance pan inertia for one frame. */
    void onUpdate(double delta_time) noexcept override;

    /** Attach camera (should be an OrthographicCamera). */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Set viewport size in pixels. */
    void setViewportSize(float width_px, float height_px) noexcept override;

    /** Reset pan inertia. */
    void resetState() noexcept override;

    // isEnabled / setEnabled inherited from CameraBehaviorBase

    // -------------------------------------------------------------------------
    // OrthoPanZoom-specific API
    // -------------------------------------------------------------------------

    /** Set scroll/pinch zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraBehaviorBase.

    /** Set pan inertia damping (>= 0). */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /** Get world units per pixel. */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept;

    /**
     * @brief Fit orthographic viewport to an AABB.
     * @param min_world AABB min corner in world space
     * @param max_world AABB max corner in world space
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

   private:
    void pan(float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void applyInertia(double delta_time) noexcept;

    // orthoCamera() inherited from CameraBehaviorBase
    // onZoomDolly() default in CameraBehaviorBase handles ortho zoom-to-cursor

    // camera_, enabled_, viewport_width_, viewport_height_ inherited from CameraBehaviorBase
    // zoom_method_, zoom_scale_, fov_zoom_speed_ inherited from CameraBehaviorBase

    float zoom_speed_ = 1.1f;
    float pan_damping_ = 10.0f;

    bool panning_ = false;
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};
};

}  // namespace vne::interaction
