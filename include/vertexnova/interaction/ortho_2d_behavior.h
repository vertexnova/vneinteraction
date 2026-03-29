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
 * @file ortho_2d_behavior.h
 * @brief Ortho2DBehavior — orthographic 2D viewport: pan, zoom-to-cursor, optional in-plane rotation.
 *
 * For orthographic viewports. Handles eBeginPan, ePanDelta, eEndPan, eZoomAtCursor, eResetView,
 * and when the input layer emits rotate actions: eBeginRotate, eRotateDelta, eEndRotate
 * (in-plane rotation about the view axis through the target). Inertia via exponential decay on
 * pan velocity.
 */

#include "vertexnova/interaction/camera_behavior_base.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Orthographic 2D pan, zoom, and optional in-plane rotation.
 *
 * Pan is in screen-to-world pixel coordinates (mouse Y-down matches scene motion). Zoom-to-cursor
 * preserves the world point under the cursor. In-plane rotation spins eye and up about the axis
 * through the target (slice normal). Inertia is applied to pan velocity via exponential decay.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API Ortho2DBehavior final : public CameraBehaviorBase {
   public:
    Ortho2DBehavior() noexcept = default;
    ~Ortho2DBehavior() noexcept override = default;

    Ortho2DBehavior(const Ortho2DBehavior&) = delete;
    Ortho2DBehavior& operator=(const Ortho2DBehavior&) = delete;
    Ortho2DBehavior(Ortho2DBehavior&&) noexcept = default;
    Ortho2DBehavior& operator=(Ortho2DBehavior&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eBeginPan, ePanDelta, eEndPan, eBeginRotate, eRotateDelta, eEndRotate,
     * eZoomAtCursor, eResetView.
     */
    bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept override;

    /** Advance pan inertia for one frame. */
    void onUpdate(double delta_time) noexcept override;

    /** Attach camera (should be an OrthographicCamera). */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Set viewport size in pixels. */
    void onResize(float width_px, float height_px) noexcept override;

    /** Reset pan inertia and interaction flags. */
    void resetState() noexcept override;

    // isEnabled / setEnabled inherited from CameraBehaviorBase

    // -------------------------------------------------------------------------
    // Ortho2D-specific API
    // -------------------------------------------------------------------------

    /** Set scroll/pinch zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraBehaviorBase.

    /** Set pan inertia damping (>= 0). */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /**
     * @brief In-plane rotation sensitivity in degrees per horizontal pixel (>= 0).
     * Matches the default feel of OrbitalCameraBehavior::rotation_speed_ (0.2 deg/px).
     */
    void setRotationSensitivityDegreesPerPixel(float deg_per_px) noexcept {
        rotation_deg_per_px_ = std::max(0.0f, deg_per_px);
    }
    [[nodiscard]] float getRotationSensitivityDegreesPerPixel() const noexcept { return rotation_deg_per_px_; }

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
    void rotateInPlane(float delta_x_px, float delta_y_px) noexcept;
    void applyInertia(double delta_time) noexcept;

    // orthoCamera() inherited from CameraBehaviorBase
    // applyDolly() default in CameraBehaviorBase handles ortho zoom-to-cursor

    float zoom_speed_ = 1.1f;
    float pan_damping_ = 10.0f;
    float rotation_deg_per_px_ = 0.2f;

    bool panning_ = false;
    bool rotating_ = false;
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};
};

}  // namespace vne::interaction
