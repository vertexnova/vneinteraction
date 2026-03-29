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
 * @file orbital_camera_behavior.h
 * @brief OrbitalCameraBehavior — orbit camera behavior with Euler or virtual-trackball rotation (ICameraBehavior).
 *
 * Supports both Euler (classic orbit) and quaternion virtual-trackball rotation modes,
 * and three pivot modes (COI, ViewCenter, Fixed). Handles rotate, pan, zoom,
 * inertia, and fitToAABB.
 */

#include "vertexnova/interaction/camera_behavior_base.h"
#include "vertexnova/interaction/orbit_behavior.h"
#include "vertexnova/interaction/trackball_behavior.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Orbital camera behavior (Euler orbit or virtual-trackball rotation).
 *
 * Implements ICameraBehavior for orbit-style interaction. Handles rotate, pan, and zoom
 * actions. Supports inertia via exponential decay.
 *
 * - RotationMode::eOrbit   — classic yaw/pitch orbit, pitch clamped to [-89°, 89°]
 * - RotationMode::eTrackball — arcball-style quaternion about COI (cf. ArcballManipulator + distance in
 *   orbit-style camera rigs)
 * - PivotMode::eCoi — orbit center follows pan in the view plane (see @ref OrbitPivotMode).
 * - PivotMode::eViewCenter — same as eCoi while panning; on pan end, COI syncs from the camera target.
 * - PivotMode::eFixed — world pivot fixed; pan trucks eye+target; after pan, target may not equal COI until rotate.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API OrbitalCameraBehavior final : public CameraBehaviorBase {
   public:
    /** Construct with default settings (eOrbit mode, eCoi pivot, Y-up). */
    OrbitalCameraBehavior() noexcept;
    ~OrbitalCameraBehavior() noexcept override = default;

    OrbitalCameraBehavior(const OrbitalCameraBehavior&) = delete;
    OrbitalCameraBehavior& operator=(const OrbitalCameraBehavior&) = delete;
    OrbitalCameraBehavior(OrbitalCameraBehavior&&) noexcept = default;
    OrbitalCameraBehavior& operator=(OrbitalCameraBehavior&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eBeginRotate, eRotateDelta, eEndRotate, eBeginPan, ePanDelta, eEndPan,
     *          eZoomAtCursor, eOrbitPanModifier, eResetView, eSetPivotAtCursor.
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

    // isEnabled / setEnabled inherited from CameraBehaviorBase

    // -------------------------------------------------------------------------
    // Orbit / trackball-specific API
    // -------------------------------------------------------------------------

    /** Set the rotation algorithm (eOrbit or eTrackball). */
    void setRotationMode(OrbitRotationMode mode) noexcept { rotation_mode_ = mode; }
    /** Get the current rotation algorithm. */
    [[nodiscard]] OrbitRotationMode getRotationMode() const noexcept { return rotation_mode_; }

    /** Trackball screen-to-sphere mapping (default: @ref TrackballBehavior::ProjectionMode::eHyperbolic). */
    void setTrackballProjectionMode(TrackballBehavior::ProjectionMode mode) noexcept {
        trackball_.setProjectionMode(mode);
    }
    [[nodiscard]] TrackballBehavior::ProjectionMode getTrackballProjectionMode() const noexcept {
        return trackball_.getProjectionMode();
    }

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

    /**
     * @brief Set the camera view-direction preset (front, back, top, iso…).
     * @details For @c eTop / @c eBottom, pitch matches @ref OrbitBehavior::getPitchMinDeg() /
     *          getPitchMaxDeg() on the internal yaw/pitch state (defaults ±89°).
     */
    void setViewDirection(ViewDirection dir) noexcept;

    /** Set orbit distance (camera-to-pivot); clamped to [0.01, 1e6]. */
    void setOrbitDistance(float distance) noexcept;
    /** Get current orbit distance. */
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }

    /** Set scroll/pinch zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraBehaviorBase.

    /**
     * Set rotation speed multiplier (>= 0). Scales Euler yaw/pitch (deg/pixel). For trackball mode, the
     * effective angle scale is rotation_speed × trackball_rotation_scale (see setTrackballRotationScale).
     */
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = std::max(0.0f, speed); }
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }

    /**
     * Extra scale applied only in @c OrbitRotationMode::eTrackball (>= 0). The trackball path scales
     * quaternion angle by rotation_speed, while Euler uses deg/pixel — the defaults match feel across
     * modes. Default 2.5.
     */
    void setTrackballRotationScale(float scale) noexcept { trackball_rotation_scale_ = std::max(0.0f, scale); }
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
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept;
    [[nodiscard]] vne::math::Vec3f computeRight(const vne::math::Vec3f& front) const noexcept;
    [[nodiscard]] vne::math::Vec3f computeUp(const vne::math::Vec3f& front,
                                             const vne::math::Vec3f& right) const noexcept;

    void syncFromCamera() noexcept;
    /** Eye at COI + orientation * distance (trackball / arcball-style), like CameraManipulator::getMatrix + look-at. */
    void applyTrackballOrbitToCamera() noexcept;
    /** Eye at COI − front * distance from yaw/pitch orbit state. */
    void applyEulerOrbitToCamera() noexcept;
    void applyToCamera() noexcept;
    void onPivotChanged() noexcept;

    void syncCoiAndDistanceFromCamera() noexcept;
    void syncTrackballOrientationFromCamera() noexcept;
    void syncEulerYawPitchFromCamera() noexcept;

    // ---- rotation ---------------------------------------------------------------
    void beginRotate(float x_px, float y_px) noexcept;
    void dragRotateEuler(float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void dragRotateTrackball(float x_px, float y_px, double delta_time) noexcept;
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
    void updateTrackballDragInertiaFromFrame(const vne::math::Vec3f& prev_sphere,
                                             const vne::math::Vec3f& curr_sphere,
                                             float trackball_rot_scale,
                                             double delta_time) noexcept;

    // ---- camera helpers ---------------------------------------------------------
    [[nodiscard]] bool isPerspective() const noexcept;
    [[nodiscard]] bool isOrthographic() const noexcept;

    // ---- state ------------------------------------------------------------------
    // camera_, enabled_, viewport_ inherited from CameraBehaviorBase
    // zoom_method_, zoom_scale_, fov_zoom_speed_ inherited from CameraBehaviorBase

    OrbitRotationMode rotation_mode_ = OrbitRotationMode::eOrbit;
    OrbitPivotMode pivot_mode_ = OrbitPivotMode::eCoi;

    // Common orbit state
    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};
    vne::math::Vec3f coi_world_{0.0f, 0.0f, 0.0f};
    float orbit_distance_ = 5.0f;

    // Euler rotation (classic yaw/pitch around world-up reference)
    OrbitBehavior orbit_behavior_;

    // Trackball (eTrackball) rotation state
    vne::math::Quatf orientation_;
    vne::math::Quatf orientation_at_drag_start_;
    TrackballBehavior trackball_;
    uint32_t normalize_counter_ = 0;
    float inertia_rot_speed_ = 0.0f;  // trackball angular speed (rad/s)
    vne::math::Vec3f inertia_rot_axis_{0.0f, 1.0f, 0.0f};

    // Pan inertia
    vne::math::Vec3f inertia_pan_velocity_{0.0f, 0.0f, 0.0f};

    // Interaction flags
    OrbitInteractionState interaction_;

    // Speeds / damping
    float rotation_speed_ = 0.2f;
    float trackball_rotation_scale_ = 2.5f;
    float pan_speed_ = 1.0f;
    float rot_damping_ = 8.0f;
    float pan_damping_ = 10.0f;
    float zoom_speed_ = 1.1f;

    // fitToAABB smooth animation
    float target_orbit_distance_ = 5.0f;
    vne::math::Vec3f target_coi_world_{0.0f, 0.0f, 0.0f};
    bool animating_fit_ = false;
};

}  // namespace vne::interaction
