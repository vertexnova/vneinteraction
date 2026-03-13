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
 * @file orbit_arcball_behavior.h
 * @brief OrbitArcballBehavior — orbit/arcball camera behavior (ICameraBehavior implementation).
 *
 * Supports both Euler (classic orbit) and Quaternion (arcball) rotation modes,
 * and three pivot modes (COI, ViewCenter, Fixed). Handles rotate, pan, zoom,
 * inertia, and fitToAABB.
 */

#include "vertexnova/interaction/camera_behavior.h"
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
 * @brief Rotation algorithm selection for OrbitArcballBehavior.
 */
enum class OrbitRotationMode : std::uint8_t {
    eEuler = 0,       //!< Classic Euler yaw/pitch orbit
    eQuaternion = 1,  //!< Arcball quaternion rotation (smooth, no gimbal lock)
};

/**
 * @brief Orbit/arcball camera behavior.
 *
 * Implements ICameraBehavior for orbit-style interaction. Handles rotate, pan, and zoom
 * actions. Supports inertia via exponential decay.
 *
 * - RotationMode::eEuler     ->classic yaw/pitch orbit, pitch clamped to [-89, 89]
 * - RotationMode::eQuaternion ->arcball quaternion rotation (unconstrained)
 * - PivotMode::eCoi          ->orbit center follows panning (standard)
 * - PivotMode::eViewCenter   ->pivot snaps to view center on pan release
 * - PivotMode::eFixed        ->pivot stays fixed; pan translates eye+target
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class OrbitArcballBehavior final : public ICameraBehavior {
   public:
    /** Construct with default settings (Euler mode, eCoi pivot, Y-up). */
    OrbitArcballBehavior() noexcept;
    ~OrbitArcballBehavior() noexcept override = default;

    OrbitArcballBehavior(const OrbitArcballBehavior&) = delete;
    OrbitArcballBehavior& operator=(const OrbitArcballBehavior&) = delete;
    OrbitArcballBehavior(OrbitArcballBehavior&&) noexcept = default;
    OrbitArcballBehavior& operator=(OrbitArcballBehavior&&) noexcept = default;

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
    void setViewportSize(float width_px, float height_px) noexcept override;

    /** Reset all interaction state (velocities, drag tracking). */
    void resetState() noexcept override;

    /** @return true if this behavior is enabled. */
    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }

    /** Enable or disable this behavior. */
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

    // -------------------------------------------------------------------------
    // Orbit/arcball-specific API
    // -------------------------------------------------------------------------

    /** Set the rotation algorithm (Euler or Quaternion). */
    void setRotationMode(OrbitRotationMode mode) noexcept { rotation_mode_ = mode; }
    /** Get the current rotation algorithm. */
    [[nodiscard]] OrbitRotationMode getRotationMode() const noexcept { return rotation_mode_; }

    /** Set the pivot control mode. */
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

    /** Set the camera view-direction preset (front, back, top, iso…). */
    void setViewDirection(ViewDirection dir) noexcept;

    /** Set orbit distance (camera-to-pivot); clamped to [0.01, 1e6]. */
    void setOrbitDistance(float distance) noexcept;
    /** Get current orbit distance. */
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }

    /** Get current scene scale (only meaningful when ZoomMethod::eSceneScale). */
    [[nodiscard]] float getSceneScale() const noexcept { return scene_scale_; }

    /** Set zoom method (dolly, scene scale, or FOV). */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /** Set scroll/pinch zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    /** Set FOV zoom speed (>= 0.01, perspective only). */
    void setFovZoomSpeed(float speed) noexcept { fov_zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }

    /** Set rotation speed multiplier (>= 0). */
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = std::max(0.0f, speed); }
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }

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

   private:
    // ---- helpers shared by both rotation modes --------------------------------
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept;
    [[nodiscard]] vne::math::Vec3f computeRight(const vne::math::Vec3f& front) const noexcept;
    [[nodiscard]] vne::math::Vec3f computeUp(const vne::math::Vec3f& front,
                                             const vne::math::Vec3f& right) const noexcept;

    void syncFromCamera() noexcept;
    void applyToCamera() noexcept;
    void onPivotChanged() noexcept;

    // ---- rotation ---------------------------------------------------------------
    void beginRotate(float x_px, float y_px) noexcept;
    void dragRotateEuler(float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void dragRotateArcball(float x_px, float y_px, double delta_time) noexcept;
    void endRotate(double delta_time) noexcept;

    // Arcball helpers
    [[nodiscard]] vne::math::Vec3f projectToArcball(float x_px, float y_px) const noexcept;

    // ---- pan --------------------------------------------------------------------
    void beginPan(float x_px, float y_px) noexcept;
    void dragPan(float x_px, float y_px, float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void endPan(double delta_time) noexcept;

    // ---- zoom -------------------------------------------------------------------
    void zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void zoomOrthoToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;

    // ---- inertia ----------------------------------------------------------------
    void applyInertia(double delta_time) noexcept;
    void doPanInertia(double delta_time) noexcept;

    // ---- camera helpers ---------------------------------------------------------
    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> perspCamera() const noexcept;
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> orthoCamera() const noexcept;
    [[nodiscard]] bool isPerspective() const noexcept;
    [[nodiscard]] bool isOrthographic() const noexcept;

    // ---- state ------------------------------------------------------------------
    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;

    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    float scene_scale_ = 1.0f;

    OrbitRotationMode rotation_mode_ = OrbitRotationMode::eEuler;
    OrbitPivotMode pivot_mode_ = OrbitPivotMode::eCoi;

    // Common orbit state
    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};
    vne::math::Vec3f coi_world_{0.0f, 0.0f, 0.0f};
    float orbit_distance_ = 5.0f;

    // Euler rotation state
    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float inertia_rot_speed_x_ = 0.0f;
    float inertia_rot_speed_y_ = 0.0f;

    // Arcball rotation state
    vne::math::Quatf orientation_;
    uint32_t normalize_counter_ = 0;
    float arcball_start_x_ = 0.0f;
    float arcball_start_y_ = 0.0f;
    float inertia_rot_speed_ = 0.0f;  // arcball angular speed (rad/s)
    vne::math::Vec3f inertia_rot_axis_{0.0f, 1.0f, 0.0f};

    // Pan inertia
    vne::math::Vec3f inertia_pan_velocity_{0.0f, 0.0f, 0.0f};

    // Interaction flags
    OrbitInteractionState interaction_;

    // Speeds / damping
    float rotation_speed_ = 0.2f;
    float pan_speed_ = 1.0f;
    float rot_damping_ = 8.0f;
    float pan_damping_ = 10.0f;
    float zoom_speed_ = 1.1f;
    float fov_zoom_speed_ = 1.05f;
    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;

    // fitToAABB smooth animation
    float target_orbit_distance_ = 5.0f;
    vne::math::Vec3f target_coi_world_{0.0f, 0.0f, 0.0f};
    bool animating_fit_ = false;
};

}  // namespace vne::interaction
