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

#include "vertexnova/interaction/camera_behavior.h"
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
class OrthoPanZoomBehavior final : public ICameraBehavior {
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

    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

    // -------------------------------------------------------------------------
    // OrthoPanZoom-specific API
    // -------------------------------------------------------------------------

    /** Set zoom method (eDollyToCoi and eChangeFov both use zoom-to-cursor for ortho). */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /** Set scroll/pinch zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

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
    void zoomToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void applyZoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void applyInertia(double delta_time) noexcept;

    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> orthoCamera() const noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;

    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    float scene_scale_ = 1.0f;

    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
    float zoom_speed_ = 1.1f;
    float pan_damping_ = 10.0f;

    bool panning_ = false;
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};
};

}  // namespace vne::interaction
