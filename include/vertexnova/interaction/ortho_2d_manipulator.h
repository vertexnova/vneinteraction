#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file ortho_2d_manipulator.h
 * @brief Ortho2DManipulator — orthographic 2D viewport: pan, zoom-to-cursor, optional in-plane rotation.
 *
 * For orthographic viewports. Handles pan, zoom, and optional in-plane rotation.
 *
 * @par Rotation
 * Rotation is in-plane (about the view axis through the camera target) and is only
 * applied when rotate actions are emitted by the input binding layer.
 *
 * @par Inertia
 * Pan velocity is damped over time in @ref onUpdate using exponential decay.
 *
 * @par Input pairing
 * @ref Ortho2DController wires @ref InputMapper rules; this manipulator handles @c ePanDelta,
 * @c eZoomAtCursor, and optional @c eBeginRotate / @c eRotateDelta / @c eEndRotate when rotation is enabled.
 */

#include "vertexnova/interaction/camera_manipulator_base.h"
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
 * Pan is in screen-to-world pixel coordinates (mouse Y-down matches scene motion).
 * Zoom-to-cursor preserves the world point under the cursor.
 * In-plane rotation spins eye/up around the axis through the target (slice normal).
 *
 * @par Action coverage
 * Pan: @c eBeginPan / @c ePanDelta / @c eEndPan
 * Rotate: @c eBeginRotate / @c eRotateDelta / @c eEndRotate
 * Zoom/reset: @c eZoomAtCursor / @c eResetView
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API Ortho2DManipulator final : public CameraManipulatorBase {
   public:
    Ortho2DManipulator() noexcept = default;
    ~Ortho2DManipulator() noexcept override = default;

    Ortho2DManipulator(const Ortho2DManipulator&) = delete;
    Ortho2DManipulator& operator=(const Ortho2DManipulator&) = delete;
    Ortho2DManipulator(Ortho2DManipulator&&) noexcept = default;
    Ortho2DManipulator& operator=(Ortho2DManipulator&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ICameraManipulator
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

    // isEnabled / setEnabled inherited from CameraManipulatorBase

    // -------------------------------------------------------------------------
    // Ortho2D-specific API
    // -------------------------------------------------------------------------

    /** Exponent on scroll factor: effective = pow(scroll_factor, zoom_speed_) before zoom dispatch (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraManipulatorBase.

    /** Set pan inertia damping (>= 0). */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /** When false, pan release does not coast. Default: true. */
    void setPanInertiaEnabled(bool enabled) noexcept { pan_inertia_enabled_ = enabled; }
    [[nodiscard]] bool isPanInertiaEnabled() const noexcept { return pan_inertia_enabled_; }

    /** When false, ignore rotate actions. */
    void setRotateEnabled(bool enabled) noexcept { rotate_enabled_ = enabled; }
    [[nodiscard]] bool isRotateEnabled() const noexcept { return rotate_enabled_; }

    /** When false, ignore pan actions. */
    void setPanEnabled(bool enabled) noexcept { pan_enabled_ = enabled; }
    [[nodiscard]] bool isPanEnabled() const noexcept { return pan_enabled_; }

    /**
     * @brief In-plane rotation sensitivity in degrees per horizontal pixel (>= 0).
     * Matches the default feel of OrbitalCameraManipulator::rotation_speed_ (0.2 deg/px).
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

    // orthoCamera() inherited from CameraManipulatorBase
    // applyDolly() default in CameraManipulatorBase handles ortho zoom-to-cursor

    float zoom_speed_ = 1.1f;
    float pan_damping_ = 10.0f;
    float rotation_deg_per_px_ = 0.2f;

    bool pan_inertia_enabled_ = true;
    bool rotate_enabled_ = true;
    bool pan_enabled_ = true;

    bool panning_ = false;
    bool rotating_ = false;
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};
};

}  // namespace vne::interaction
