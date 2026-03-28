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
 * @file free_look_behavior.h
 * @brief FreeLookBehavior — FPS / Fly camera behavior (ICameraBehavior implementation).
 *
 * WASD movement + mouse look. Two modes:
 * - FreeLookMode::eFps — world up fixed, pitch clamped [-89°, 89°]
 * - FreeLookMode::eFly  — unconstrained, up follows camera
 */

#include "vertexnova/interaction/camera_behavior_base.h"
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

/** Movement mode for FreeLookBehavior. */
enum class FreeLookMode : std::uint8_t {
    eFps = 0,  //!< FPS: world-up fixed, pitch clamped [-89°, 89°] (default)
    eFly = 1,  //!< Fly: unconstrained, up follows camera
};

/**
 * @brief Free-look camera behavior supporting FPS and Fly movement modes.
 *
 * Handles WASD movement + mouse-look + zoom actions. Two sub-modes:
 *
 * - **eFps** — world_up fixed, pitch clamped to [-89°, 89°].
 * - **eFly** — up vector tracks camera orientation, no pitch constraint.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API FreeLookBehavior final : public CameraBehaviorBase {
   public:
    /** Construct with default FPS settings (FreeLookMode::eFps, Y-up). */
    FreeLookBehavior() noexcept = default;
    ~FreeLookBehavior() noexcept override = default;

    FreeLookBehavior(const FreeLookBehavior&) = delete;
    FreeLookBehavior& operator=(const FreeLookBehavior&) = delete;
    FreeLookBehavior(FreeLookBehavior&&) noexcept = default;
    FreeLookBehavior& operator=(FreeLookBehavior&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eBeginLook, eEndLook, eLookDelta, eMoveForward/Backward/Left/Right/Up/Down,
     *          eSprintModifier, eSlowModifier, eZoomAtCursor, eResetView.
     */
    bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept override;

    /** Advance WASD movement for one frame. */
    void onUpdate(double delta_time) noexcept override;

    /** Attach camera; syncs yaw/pitch from current camera orientation. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Set viewport size in pixels. */
    void onResize(float width_px, float height_px) noexcept override;

    /** Reset all input state (keys, looking flag). */
    void resetState() noexcept override;

    // isEnabled / setEnabled inherited from CameraBehaviorBase

    // -------------------------------------------------------------------------
    // FreeLook-specific API
    // -------------------------------------------------------------------------

    /** Set movement mode (eFps or eFly). */
    void setMode(FreeLookMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] FreeLookMode getMode() const noexcept { return mode_; }

    /**
     * @brief Convenience: set mode from bool (true = eFps, false = eFly).
     * @param constrain true = FPS; false = Fly
     */
    void setConstrainWorldUp(bool constrain) noexcept { mode_ = constrain ? FreeLookMode::eFps : FreeLookMode::eFly; }
    [[nodiscard]] bool getConstrainWorldUp() const noexcept { return mode_ == FreeLookMode::eFps; }

    /** Set the world-space up vector used in FPS mode (default: +Y). */
    void setWorldUp(const vne::math::Vec3f& up) noexcept;
    [[nodiscard]] vne::math::Vec3f getWorldUp() const noexcept { return world_up_; }

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

    /** Set zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    // setZoomMethod / getZoomMethod / setFovZoomSpeed / getFovZoomSpeed / getZoomScale
    // are inherited from CameraBehaviorBase.

    /**
     * @brief Enable or disable zoom handling for this behavior (default: true).
     * Disable when another behavior in the same rig (e.g. OrbitalCameraBehavior)
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

   private:
    // ---- up-vector policy (FPS vs Fly) --------------------------------------
    [[nodiscard]] vne::math::Vec3f upVector() const noexcept;

    // ---- movement math -------------------------------------------------------
    [[nodiscard]] vne::math::Vec3f front() const noexcept;
    [[nodiscard]] vne::math::Vec3f right(const vne::math::Vec3f& front_vec) const noexcept;

    void syncAnglesFromCamera() noexcept;
    void applyAnglesToCamera() noexcept;
    void onZoomDolly(float factor, float mx, float my) noexcept override;

    // perspCamera() / orthoCamera() inherited from CameraBehaviorBase

    // ---- state ---------------------------------------------------------------
    // camera_, enabled_, viewport_ inherited from CameraBehaviorBase
    // zoom_method_, zoom_scale_, fov_zoom_speed_ inherited from CameraBehaviorBase

    FreeLookMode mode_ = FreeLookMode::eFps;
    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};

    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float move_speed_ = 3.0f;
    float mouse_sensitivity_ = 0.15f;
    float sprint_mult_ = 4.0f;
    float slow_mult_ = 0.2f;
    float zoom_speed_ = 0.5f;
    bool handle_zoom_ = true;

    FreeLookInputState input_state_;
};

}  // namespace vne::interaction
