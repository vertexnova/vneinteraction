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
 * @file camera_rig.h
 * @brief CameraRig — multi-behavior camera container.
 *
 * A CameraRig holds one or more ICameraBehavior instances that each receive
 * the same CameraAction stream independently. Use the static make*() factories
 * for common configurations, or compose behaviors manually for custom setups.
 */

#include "vertexnova/interaction/camera_behavior.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera.h"

#include <memory>
#include <vector>

namespace vne::interaction {

/**
 * @brief Multi-behavior camera container.
 *
 * All registered behaviors receive every CameraAction. Each behavior silently
 * ignores actions it doesn't handle. This enables composition: e.g. adding
 * both OrbitBehavior and FreeLookBehavior creates a "game camera" that
 * supports both orbiting and WASD flight simultaneously.
 *
 * ### Usage
 * ```cpp
 * // Built-in presets:
 * auto rig = CameraRig::makeOrbit();
 *
 * // Custom composition (game camera):
 * CameraRig rig;
 * rig.addBehavior(std::make_shared<OrbitBehavior>());
 * rig.addBehavior(std::make_shared<FreeLookBehavior>());
 * ```
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API CameraRig {
   public:
    CameraRig() = default;
    ~CameraRig() = default;

    CameraRig(const CameraRig&) = delete;
    CameraRig& operator=(const CameraRig&) = delete;
    CameraRig(CameraRig&&) noexcept = default;
    CameraRig& operator=(CameraRig&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Behavior management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a behavior to the rig.
     * @param behavior Shared pointer to behavior; must not be nullptr
     */
    void addBehavior(std::shared_ptr<ICameraBehavior> behavior);

    /**
     * @brief Remove a previously added behavior by pointer identity.
     * @param behavior The behavior to remove; no-op if not found
     */
    void removeBehavior(const std::shared_ptr<ICameraBehavior>& behavior);

    /** Remove all behaviors. */
    void clearBehaviors();

    /** @return Read-only view of all registered behaviors. */
    [[nodiscard]] const std::vector<std::shared_ptr<ICameraBehavior>>& behaviors() const noexcept { return behaviors_; }

    // -------------------------------------------------------------------------
    // Action dispatch (called by CameraSystemController)
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch an action to all registered behaviors.
     * @param action     The semantic camera action
     * @param payload    Position/delta/zoom data
     * @param delta_time Time since last input in seconds
     */
    void onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept;

    /**
     * @brief Advance all behaviors by one frame.
     * @param delta_time Elapsed time in seconds since last frame
     */
    void onUpdate(double delta_time) noexcept;

    /**
     * @brief Set the controlled camera on all behaviors.
     * @param camera Shared pointer to the camera; may be nullptr to detach
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept;

    /**
     * @brief Notify all behaviors of the current viewport dimensions.
     * @param width_px  Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    void setViewportSize(float width_px, float height_px) noexcept;

    /** Reset all behavior states. */
    void resetState() noexcept;

    // -------------------------------------------------------------------------
    // Convenience factory methods
    // -------------------------------------------------------------------------

    /** Orbit rig: OrbitBehavior with Euler rotation. Default for 3D model viewers. */
    static CameraRig makeOrbit();

    /** Arcball rig: OrbitBehavior with Quaternion rotation. Smoother, no gimbal lock. */
    static CameraRig makeArcball();

    /** FPS rig: FreeLookBehavior with world-up constraint. */
    static CameraRig makeFps();

    /** Fly rig: FreeLookBehavior unconstrained (no world-up, allows barrel roll). */
    static CameraRig makeFly();

    /** Ortho pan+zoom rig: OrthoPanZoomBehavior. */
    static CameraRig makeOrthoPanZoom();

    /** Follow rig: TrackBehavior (autonomous smooth target following). */
    static CameraRig makeFollow();

    /**
     * Game camera rig: OrbitBehavior (Euler) + FreeLookBehavior (constrained).
     * LMB orbits, RMB looks, WASD flies — all simultaneously.
     */
    static CameraRig makeGameCamera();

    /** 2D rig: OrthoPanZoomBehavior (alias for makeOrthoPanZoom). */
    static CameraRig make2D();

   private:
    std::vector<std::shared_ptr<ICameraBehavior>> behaviors_;
};

}  // namespace vne::interaction
