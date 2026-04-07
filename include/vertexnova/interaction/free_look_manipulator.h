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
 * @file free_look_manipulator.h
 * @brief FreeLookManipulator — FPS / Fly camera (ICameraManipulator implementation).
 *
 * Orientation is stored as a **camera-to-world quaternion** and applied via
 * @c ICamera::setOrientationView (vnescene quaternion-primary camera). Yaw/pitch accessors are
 * derived for compatibility and bookmarks.
 *
 * @par Movement model
 * WASD movement + mouse look. Two modes (see @ref FreeLookMode in interaction_types.h):
 * - FreeLookMode::eFps — world up fixed, pitch clamped [-89°, 89°]
 * - FreeLookMode::eFly  — unconstrained, up follows camera
 *
 * @par Zoom
 * Zoom is dispatched through @ref CameraManipulatorBase and can be disabled per-instance
 * with @ref setHandleZoom when another manipulator should own scroll/pinch in a shared rig.
 */

#include "vertexnova/interaction/camera_manipulator_base.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

class TrackballBehavior;

/**
 * @brief Free-look camera manipulator supporting FPS and Fly movement modes.
 *
 * Handles WASD movement + mouse-look + zoom actions.
 *
 * - **eFps** — world_up fixed, pitch clamped to [-89°, 89°].
 * - **eFly** — up vector tracks camera orientation, no pitch constraint.
 *
 * @par Action coverage
 * Look: @c eBeginLook / @c eLookDelta / @c eEndLook
 * Move: @c eMoveForward / @c eMoveBackward / @c eMoveLeft / @c eMoveRight / @c eMoveUp / @c eMoveDown
 * Modifiers: @c eSprintModifier / @c eSlowModifier
 * Zoom/reset: @c eZoomAtCursor / @c eResetView
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API FreeLookManipulator final : public CameraManipulatorBase {
   public:
    /** Construct with default FPS settings (FreeLookMode::eFps, Y-up). */
    FreeLookManipulator() noexcept;
    ~FreeLookManipulator() noexcept override;

    FreeLookManipulator(const FreeLookManipulator&) = delete;
    FreeLookManipulator& operator=(const FreeLookManipulator&) = delete;
    FreeLookManipulator(FreeLookManipulator&&) noexcept;
    FreeLookManipulator& operator=(FreeLookManipulator&&) noexcept;

    // -------------------------------------------------------------------------
    // ICameraManipulator
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eBeginLook, eEndLook, eLookDelta, eMoveForward/Backward/Left/Right/Up/Down,
     *          eSprintModifier, eSlowModifier, eZoomAtCursor, eResetView.
     */
    bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept override;

    /** Advance WASD movement for one frame. */
    void onUpdate(double delta_time) noexcept override;

    /** Attach camera; syncs internal quaternion from @c ICamera::getOrientation(). */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Set viewport size in pixels. */
    void onResize(float width_px, float height_px) noexcept override;

    /** Reset all input state (keys, looking flag) and re-sync orientation from the camera if attached. */
    void resetState() noexcept override;

    // isEnabled / setEnabled inherited from CameraManipulatorBase

    // -------------------------------------------------------------------------
    // FreeLook-specific API
    // -------------------------------------------------------------------------

    /** Set movement mode (eFps or eFly). */
    void setMode(FreeLookMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] FreeLookMode getMode() const noexcept { return mode_; }

    /** Mouse-look style: yaw/pitch deltas or virtual trackball quaternion. */
    void setRotationMode(FreeLookRotationMode mode) noexcept;
    [[nodiscard]] FreeLookRotationMode getRotationMode() const noexcept { return rotation_mode_; }

    /** Trackball sphere mapping when @ref getRotationMode is @c eTrackball. */
    void setTrackballProjectionMode(TrackballProjectionMode mode) noexcept;
    [[nodiscard]] TrackballProjectionMode getTrackballProjectionMode() const noexcept;

    /**
     * @brief Convenience: set mode from bool (true = eFps, false = eFly).
     * @param constrain true = FPS; false = Fly
     */
    void setConstrainWorldUp(bool constrain) noexcept { mode_ = constrain ? FreeLookMode::eFps : FreeLookMode::eFly; }
    [[nodiscard]] bool getConstrainWorldUp() const noexcept { return mode_ == FreeLookMode::eFps; }

    /** Set the world-space up vector used in FPS mode (default: +Y). */
    void setWorldUp(const vne::math::Vec3f& up) noexcept;
    [[nodiscard]] vne::math::Vec3f getWorldUp() const noexcept { return world_up_; }

    /** Yaw in degrees derived from the stored quaternion (FPS: about @c world_up_; fly: camera-up plane). */
    [[nodiscard]] float getYawDegrees() const noexcept;
    /** Pitch in degrees derived from the stored quaternion. */
    [[nodiscard]] float getPitchDegrees() const noexcept;
    /**
     * Set yaw/pitch in degrees and refresh the camera via @c lookAt (FPS clamps pitch to [-89°, 89°]).
     */
    void setYawPitchDegrees(float yaw_deg, float pitch_deg) noexcept;

    /** Camera-to-world orientation used for mouse look and movement (normalised on set). */
    [[nodiscard]] vne::math::Quatf getOrientation() const noexcept { return orientation_; }
    void setOrientation(const vne::math::Quatf& q) noexcept;

    /** Set movement speed in world units per second (default: 3.0). */
    void setMoveSpeed(float speed) noexcept { move_speed_ = std::max(0.0f, speed); }
    [[nodiscard]] float getMoveSpeed() const noexcept { return move_speed_; }

    /** Set mouse look sensitivity in degrees per pixel (default: 0.15). */
    void setMouseSensitivity(float sensitivity) noexcept { mouse_sensitivity_ = std::max(0.0f, sensitivity); }
    [[nodiscard]] float getMouseSensitivity() const noexcept { return mouse_sensitivity_; }

    /** Set sprint speed multiplier (default: 4.0). */
    void setSprintMultiplier(float mult) noexcept { sprint_mult_ = std::max(1.0f, mult); }
    [[nodiscard]] float getSprintMultiplier() const noexcept { return sprint_mult_; }

    /** Set slow-walk speed multiplier (default: 0.2). */
    void setSlowMultiplier(float mult) noexcept { slow_mult_ = std::max(0.0f, mult); }
    [[nodiscard]] float getSlowMultiplier() const noexcept { return slow_mult_; }

    /**
     * Zoom exponent (>= 0.01), same semantics as OrbitalCameraManipulator::setZoomSpeed: effective =
     * pow(scroll_factor, zoom_speed_) from the mapper’s multiplicative factor. Perspective dolly moves along
     * view by (1 − effective) × eye–target distance per step. Values > 1 amplify wheel zoom; < 1 attenuate.
     */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraManipulatorBase.

    /**
     * @brief Enable or disable zoom handling for this manipulator (default: true).
     * Disable when another manipulator in the same rig (e.g. OrbitalCameraManipulator)
     * should own eZoomAtCursor to avoid double-zoom per scroll tick.
     */
    void setHandleZoom(bool enable) noexcept { handle_zoom_ = enable; }
    [[nodiscard]] bool getHandleZoom() const noexcept { return handle_zoom_; }

    /** Get world units per pixel (FPS: at unit distance from camera). */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept;

    /**
     * @brief Fit camera to an AABB (places camera outside the bounding sphere).
     * @param min_world AABB min corner
     * @param max_world AABB max corner
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

    /** Mark orientation as stale (e.g. external camera move). Next @ref ensureAnglesSynced re-reads the camera. */
    void markAnglesDirty() noexcept { orientation_dirty_ = true; }

   private:
    // ---- up-vector policy (FPS vs Fly) --------------------------------------
    [[nodiscard]] vne::math::Vec3f upVector() const noexcept;

    // ---- movement math -------------------------------------------------------
    [[nodiscard]] vne::math::Vec3f front() const noexcept;
    [[nodiscard]] vne::math::Vec3f right(const vne::math::Vec3f& front_vec) const noexcept;
    /** Camera up projected onto the plane perpendicular to view (for ortho WASD pan). */
    [[nodiscard]] vne::math::Vec3f orthoPanUp(const vne::math::Vec3f& view_dir) const noexcept;

    /**
     * Image-plane "up" for W/S from @p view_dir, using @p vertical_hint in degenerate fallbacks (e.g. world up for
     * orthographic keyboard pan instead of fly-mode camera up).
     */
    [[nodiscard]] vne::math::Vec3f orthoPanUp(const vne::math::Vec3f& view_dir,
                                              const vne::math::Vec3f& vertical_hint) const noexcept;

    /**
     * Orthographic cameras: image-plane up for W/S (view from @p ortho), fallbacks use @p vertical_hint.
     */
    [[nodiscard]] vne::math::Vec3f orthoPanUp(const vne::scene::OrthographicCamera& ortho,
                                              const vne::math::Vec3f& vertical_hint) const noexcept;

    void syncOrientationFromCamera() noexcept;
    /** If @ref orientation_dirty_ and a camera is set, sync quaternion from the camera and clear the flag. */
    void ensureAnglesSynced() noexcept;
    void applyOrientationToCamera() noexcept;
    void yawPitchFromOrientation(float& yaw_deg_out, float& pitch_deg_out) const noexcept;
    void clampFpsPitch() noexcept;
    void applyDolly(float factor, float mx, float my) noexcept override;

    // perspCamera() / orthoCamera() inherited from CameraManipulatorBase

    // ---- state ---------------------------------------------------------------
    // camera_, enabled_, viewport_ inherited from CameraManipulatorBase
    // zoom_method_, zoom_scale_, fov_zoom_speed_ inherited from CameraManipulatorBase

    FreeLookMode mode_ = FreeLookMode::eFps;
    FreeLookRotationMode rotation_mode_ = FreeLookRotationMode::eYawPitch;
    TrackballProjectionMode trackball_projection_mode_ = TrackballProjectionMode::eHyperbolic;
    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};

    vne::math::Quatf orientation_{0.0f, 0.0f, 0.0f, 1.0f};
    vne::math::Quatf orientation_at_drag_start_{0.0f, 0.0f, 0.0f, 1.0f};

    // TrackballBehavior is non-copyable; stored by pointer so FreeLookManipulator stays movable.
    std::unique_ptr<TrackballBehavior> trackball_;
    float move_speed_ = 3.0f;
    float mouse_sensitivity_ = 0.15f;
    float sprint_mult_ = 4.0f;
    float slow_mult_ = 0.2f;
    float zoom_speed_ = 1.1f;
    bool handle_zoom_ = true;
    bool orientation_dirty_ = true;

    FreeLookInputState input_state_;
};

}  // namespace vne::interaction
