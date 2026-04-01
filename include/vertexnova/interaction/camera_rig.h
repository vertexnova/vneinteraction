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
 * @brief CameraRig — multi-manipulator camera container.
 *
 * A CameraRig holds one or more ICameraManipulator instances that each receive
 * the same CameraAction stream independently. Use the static make*() factories
 * for common configurations, or compose manipulators manually for custom setups.
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera.h"

#include <memory>
#include <vector>

namespace vne::interaction {

/**
 * @brief Multi-manipulator camera container.
 *
 * All registered manipulators receive every CameraAction. Each manipulator silently
 * ignores actions it doesn't handle. This enables composition: e.g. adding
 * both OrbitalCameraManipulator and FreeLookManipulator creates a "game camera" that
 * supports both orbiting and WASD flight simultaneously.
 *
 * ### Usage
 * ```cpp
 * // Built-in presets:
 * auto rig = CameraRig::makeOrbit();
 *
 * // Custom composition (game camera):
 * CameraRig rig;
 * rig.addManipulator(std::make_shared<OrbitalCameraManipulator>());
 * rig.addManipulator(std::make_shared<FreeLookManipulator>());
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
    // Manipulator management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a manipulator to the rig.
     * @param manipulator Shared pointer to manipulator; must not be nullptr
     */
    void addManipulator(std::shared_ptr<ICameraManipulator> manipulator);

    /**
     * @brief Remove a previously added manipulator by pointer identity.
     * @param manipulator The manipulator to remove; no-op if not found
     */
    void removeManipulator(const std::shared_ptr<ICameraManipulator>& manipulator);

    /** Remove all manipulators. */
    void clearManipulators();

    /** @return Read-only view of all registered manipulators. */
    [[nodiscard]] const std::vector<std::shared_ptr<ICameraManipulator>>& manipulators() const noexcept {
        return manipulators_;
    }

    // -------------------------------------------------------------------------
    // Action dispatch (called by controllers)
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch an action to all registered manipulators.
     * @param action     The semantic camera action
     * @param payload    Position/delta/zoom data
     * @param delta_time Time since last input in seconds
     */
    void onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept;

    /**
     * @brief Advance all manipulators by one frame.
     * @param delta_time Elapsed time in seconds since last frame
     */
    void onUpdate(double delta_time) noexcept;

    /**
     * @brief Set the controlled camera on all manipulators.
     * @param camera Shared pointer to the camera; may be nullptr to detach
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept;

    /**
     * @brief Notify all manipulators of the current viewport dimensions.
     * @param width_px  Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    void onResize(float width_px, float height_px) noexcept;

    /** Reset all manipulator states. */
    void resetState() noexcept;

    // -------------------------------------------------------------------------
    // Convenience factory methods
    // -------------------------------------------------------------------------

    /** Orbit rig: OrbitalCameraManipulator with Euler rotation. Default for 3D model viewers. */
    static CameraRig makeOrbit();

    /** Trackball rig: OrbitalCameraManipulator with quaternion rotation. Smoother, no gimbal lock. */
    static CameraRig makeTrackball();

    /** FPS rig: FreeLookManipulator with world-up constraint. */
    static CameraRig makeFps();

    /** Fly rig: FreeLookManipulator unconstrained (no world-up, allows barrel roll). */
    static CameraRig makeFly();

    /** 2D orthographic rig: Ortho2DManipulator (pan, zoom, optional in-plane rotation). */
    static CameraRig makeOrtho2D();

    /** Follow rig: FollowManipulator (autonomous smooth target following). */
    static CameraRig makeFollow();

    /** 2D rig: Ortho2DManipulator (alias for makeOrtho2D). */
    static CameraRig make2D();

   private:
    std::vector<std::shared_ptr<ICameraManipulator>> manipulators_;
};

}  // namespace vne::interaction
