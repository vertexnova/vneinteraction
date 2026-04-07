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
 * @file orbital_camera_manipulator.h
 * @brief OrbitalCameraManipulator — quaternion virtual-trackball orbit camera manipulator.
 *
 * Public orbit manipulator used by inspect-style controllers. Trackball sphere mapping is
 * implemented by internal helpers (\c src/vertexnova/interaction/detail/trackball_behavior.h);
 * they are not installed as public API types.
 *
 * @par Pivot modes
 * - @c OrbitPivotMode::eCoi: pivot follows pan in view plane.
 * - @c OrbitPivotMode::eViewCenter: behaves like COI while panning, then recenters from camera target on pan end.
 * - @c OrbitPivotMode::eFixed: fixed world pivot; pan trucks eye+target.
 *
 * @par Zoom
 * Zoom dispatch is inherited from @ref CameraManipulatorBase and supports
 * @ref ZoomMethod::eDollyToCoi, @ref ZoomMethod::eSceneScale, and @ref ZoomMethod::eChangeFov.
 */

#include "vertexnova/interaction/camera_manipulator_base.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {
class IRotationStrategy;
}  // namespace vne::interaction

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Orbit manipulator with Euler/trackball rotation, pan, zoom, inertia, and fit.
 *
 * Handles action-driven interaction and writes the resulting pose to the attached
 * @ref vne::scene::ICamera.
 *
 * @par Action coverage
 * Rotate: @c eBeginRotate / @c eRotateDelta / @c eEndRotate
 * Pan: @c eBeginPan / @c ePanDelta / @c eEndPan
 * Zoom: @c eZoomAtCursor
 * Utility: @c eResetView, @c eSetPivotAtCursor, @c eOrbitPanModifier
 *
 * @par Inertia
 * Rotation and pan both support damping-based inertia via @ref onUpdate.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API OrbitalCameraManipulator final : public CameraManipulatorBase {
   public:
    /** Construct with default settings (trackball rotation, eCoi pivot, Y-up). */
    OrbitalCameraManipulator() noexcept;
    ~OrbitalCameraManipulator() noexcept override;

    OrbitalCameraManipulator(const OrbitalCameraManipulator&) = delete;
    OrbitalCameraManipulator& operator=(const OrbitalCameraManipulator&) = delete;
    // Move defined in .cpp: std::unique_ptr<IRotationStrategy> requires a complete type there (MSVC).
    OrbitalCameraManipulator(OrbitalCameraManipulator&&) noexcept;
    OrbitalCameraManipulator& operator=(OrbitalCameraManipulator&&) noexcept;

    // -------------------------------------------------------------------------
    // ICameraManipulator
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eBeginRotate, eRotateDelta, eEndRotate, eBeginPan, ePanDelta, eEndPan,
     *          eZoomAtCursor, eOrbitPanModifier, eResetView, eSetPivotAtCursor (COI on view ray; ignores payload x/y).
     */
    bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept override;

    /** Advance inertia (rotation and pan) and fitToAABB animation. */
    void onUpdate(double delta_time) noexcept override;

    /** Attach camera; syncs internal state from camera. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Set viewport size in pixels for screen-to-world projection. */
    void onResize(float width_px, float height_px) noexcept override;

    /** Reset all interaction state (velocities, drag tracking). */
    void resetState() noexcept override;

    // isEnabled / setEnabled inherited from CameraManipulatorBase

    // -------------------------------------------------------------------------
    // Orbit / trackball-specific API
    // -------------------------------------------------------------------------

    /** Trackball screen-to-sphere mapping (default: @ref TrackballProjectionMode::eHyperbolic). */
    void setTrackballProjectionMode(TrackballProjectionMode mode) noexcept;
    [[nodiscard]] TrackballProjectionMode getTrackballProjectionMode() const noexcept;

    /** Set the pivot control mode (@ref OrbitPivotMode). */
    void setPivotMode(OrbitPivotMode mode) noexcept { pivot_mode_ = mode; }
    /** Get the current pivot mode. */
    [[nodiscard]] OrbitPivotMode getPivotMode() const noexcept { return pivot_mode_; }

    /**
     * @brief Set the orbit pivot to a fixed world-space position.
     * Equivalent to: setCenterOfInterest(pt), setPivotMode(eFixed).
     */
    void setLandmark(const vne::math::Vec3f& world_pos) noexcept;

    /**
     * @brief Set the center of interest (orbit pivot) in world or camera space.
     * Switches pivot mode to eCoi (not eFixed).
     */
    void setPivot(const vne::math::Vec3f& pos,
                  CenterOfInterestSpace space = CenterOfInterestSpace::eWorldSpace) noexcept;

    /** Get the current center of interest in world space. */
    [[nodiscard]] vne::math::Vec3f getCenterOfInterestWorld() const noexcept { return coi_world_; }

    /** Set the world-space up vector (default: +Y). */
    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;
    /** World up used for orbit framing (same convention as @ref setWorldUp). */
    [[nodiscard]] vne::math::Vec3f getWorldUp() const noexcept { return world_up_; }

    /** Get the current trackball orientation quaternion. */
    [[nodiscard]] vne::math::Quatf getOrientation() const noexcept;
    /** Replace the orientation quaternion and update the camera. Clears rotation inertia. */
    void setOrientation(const vne::math::Quatf& rotation) noexcept;

    /**
     * @brief Set the camera view-direction preset (front, back, top, iso…).
     * @details For @c eTop / @c eBottom, pitch uses the same default polar limits as the internal
     *          Euler path (±89° on the yaw/pitch state inside the implementation).
     */
    void setViewDirection(ViewDirection dir) noexcept;

    /** Set orbit distance (camera-to-pivot); clamped to [0.01, 1e6]. */
    void setOrbitDistance(float distance) noexcept;
    /** Get current orbit distance. */
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }

    /**
     * Set zoom sensitivity exponent (>= 0.01). Applied as pow(scroll_factor, zoom_speed_) before changing
     * orbit distance / ortho extents (dolly path). Values > 1 amplify wheel zoom; < 1 attenuate. Default 1.1.
     */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraManipulatorBase.

    /**
     * Set rotation speed multiplier (>= 0). The effective trackball angle scale is
     * rotation_speed × trackball_rotation_scale (see setTrackballRotationScale).
     */
    void setRotationSpeed(float speed) noexcept;
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }

    /**
     * Extra scale applied to the trackball quaternion angle (>= 0). Effective rotation angle =
     * rotation_speed × trackball_rotation_scale. Default 2.5.
     */
    void setTrackballRotationScale(float scale) noexcept;
    [[nodiscard]] float getTrackballRotationScale() const noexcept { return trackball_rotation_scale_; }

    /** Set pan speed multiplier (>= 0). */
    void setPanSpeed(float speed) noexcept { pan_speed_ = std::max(0.0f, speed); }
    [[nodiscard]] float getPanSpeed() const noexcept { return pan_speed_; }

    /** Set rotation inertia damping (>= 0; higher = faster stop). */
    void setRotationDamping(float damping) noexcept { rot_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getRotationDamping() const noexcept { return rot_damping_; }

    /** Set pan inertia damping (>= 0). */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /** When false, rotation drag/release does not coast (Euler and trackball). Default: true. */
    void setRotationInertiaEnabled(bool enabled) noexcept { rotation_inertia_enabled_ = enabled; }
    [[nodiscard]] bool isRotationInertiaEnabled() const noexcept { return rotation_inertia_enabled_; }

    /** When false, pan drag/release does not coast. Default: true. */
    void setPanInertiaEnabled(bool enabled) noexcept { pan_inertia_enabled_ = enabled; }
    [[nodiscard]] bool isPanInertiaEnabled() const noexcept { return pan_inertia_enabled_; }

    /** When false, ignore rotate actions. Zoom uses manipulator @c setEnabled. */
    void setRotateEnabled(bool enabled) noexcept { rotate_enabled_ = enabled; }
    [[nodiscard]] bool isRotateEnabled() const noexcept { return rotate_enabled_; }

    /** When false, ignore pan actions. */
    void setPanEnabled(bool enabled) noexcept { pan_enabled_ = enabled; }
    [[nodiscard]] bool isPanEnabled() const noexcept { return pan_enabled_; }

    /**
     * @brief Fit camera to an AABB with smooth animation.
     * @param min_world AABB min corner in world space
     * @param max_world AABB max corner in world space
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

    /** Get world units per pixel (useful for screen-to-world conversions). */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept;

   protected:
    /**
     * @brief Zoom dispatch: eSceneScale → applySceneScaleZoom; eChangeFov → applyFovZoom only (no dolly
     * fallback at FOV limits — use eDollyToCoi for orbit distance zoom); eDollyToCoi → applyDolly.
     */
    void dispatchZoom(float factor, float mx, float my) noexcept;

   private:
    // ---- helpers shared by both rotation modes --------------------------------
    [[nodiscard]] vne::math::Vec3f computeFront() noexcept;
    [[nodiscard]] vne::math::Vec3f computeRight(const vne::math::Vec3f& front) const noexcept;
    [[nodiscard]] vne::math::Vec3f computeUp(const vne::math::Vec3f& front,
                                             const vne::math::Vec3f& right) const noexcept;

    void syncFromCamera() noexcept;
    void applyToCamera() noexcept;
    void onPivotChanged() noexcept;

    void syncCoiAndDistanceFromCamera() noexcept;

    // ---- rotation ---------------------------------------------------------------
    void beginRotate(float x_px, float y_px) noexcept;
    void dragRotate(float x_px, float y_px, float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void endRotate(double delta_time) noexcept;

    // ---- pan --------------------------------------------------------------------
    void beginPan(float x_px, float y_px) noexcept;
    void dragPan(float x_px, float y_px, float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void endPan(double delta_time) noexcept;

    /** Apply a world-space pan delta (eFixed vs COI/ViewCenter paths). */
    void applyPanDeltaWorld(const vne::math::Vec3f& delta_world) noexcept;

    /** EMA pan velocity from drag; skips update if @a delta_time is invalid for sampling. */
    void updatePanInertiaFromDragSample(const vne::math::Vec3f& delta_world, double delta_time) noexcept;

    // ---- zoom -------------------------------------------------------------------
    /** Ortho zoom-to-cursor or perspective orbit dolly (zoom_speed_ applied via pow). */
    void applyDolly(float factor, float mx, float my) noexcept override;

    // ---- inertia ----------------------------------------------------------------
    void applyInertia(double delta_time) noexcept;
    void doPanInertia(double delta_time) noexcept;

    // ---- camera helpers ---------------------------------------------------------
    [[nodiscard]] bool isPerspective() const noexcept;
    [[nodiscard]] bool isOrthographic() const noexcept;

    // ---- state ------------------------------------------------------------------
    // camera_, enabled_, viewport_ inherited from CameraManipulatorBase
    // zoom_method_, zoom_scale_, fov_zoom_speed_ inherited from CameraManipulatorBase

    OrbitPivotMode pivot_mode_ = OrbitPivotMode::eCoi;

    // Common orbit state
    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};
    vne::math::Vec3f coi_world_{0.0f, 0.0f, 0.0f};
    float orbit_distance_ = 5.0f;

    // Rotation strategy (Euler or Trackball); replaces per-mode state fields.
    std::unique_ptr<IRotationStrategy> rotation_strategy_;

    /** Trackball sphere mapping; cached for strategy rebuild and pre-switch configuration. */
    TrackballProjectionMode trackball_projection_mode_ = TrackballProjectionMode::eHyperbolic;

    // Pan inertia
    vne::math::Vec3f inertia_pan_velocity_{0.0f, 0.0f, 0.0f};

    // Interaction flags
    OrbitalInteractionState interaction_;

    // Speeds / damping
    float rotation_speed_ = 0.2f;
    float trackball_rotation_scale_ = 2.5f;
    float pan_speed_ = 1.0f;
    float rot_damping_ = 8.0f;
    float pan_damping_ = 10.0f;
    float zoom_speed_ = 1.1f;

    bool rotation_inertia_enabled_ = true;
    bool pan_inertia_enabled_ = true;
    bool rotate_enabled_ = true;
    bool pan_enabled_ = true;

    // fitToAABB smooth animation
    float target_orbit_distance_ = 5.0f;
    vne::math::Vec3f target_coi_world_{0.0f, 0.0f, 0.0f};
    bool animating_fit_ = false;
};

}  // namespace vne::interaction
