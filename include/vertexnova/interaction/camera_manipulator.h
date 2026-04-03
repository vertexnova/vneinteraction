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
 * @file camera_manipulator.h
 * @brief ICameraManipulator — composable piece of camera motion (orbit, fly, ortho, follow, …).
 *
 * @par Composition
 * Register several implementations on one @ref CameraRig; each receives every @ref CameraActionType
 * in @ref CameraRig::onAction and silently ignores what it does not implement.
 *
 * @par Math vs motion
 * Concrete manipulators implement motion using internal helpers and/or free-flight math
 * (orbit/trackball pieces under \c src/vertexnova/interaction/detail/ in sources). They read
 * @ref CameraCommandPayload and write @c vne::scene::ICamera poses; those internal helpers are not
 * part of the installed public API.
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Interface for a composable camera manipulator.
 *
 * Each concrete manipulator handles the subset of CameraActionType values relevant
 * to it and silently ignores the rest. Multiple manipulators can be active
 * simultaneously on a CameraRig — they each receive every action independently.
 *
 * Concrete implementations:
 *  - OrbitalCameraManipulator — rotate/pan/zoom around a pivot (Euler or Quaternion mode)
 *  - FreeLookManipulator — WASD movement + mouse look (FPS or unconstrained Fly mode)
 *  - Ortho2DManipulator — orthographic 2D pan, zoom, optional in-plane rotation
 *  - FollowManipulator   — autonomous smooth target following
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API ICameraManipulator {
   public:
    virtual ~ICameraManipulator() noexcept = default;

    /**
     * @brief Dispatch a camera action to this manipulator.
     *
     * Called by CameraRig for every action in the stream. Manipulators silently
     * ignore actions they don't handle.
     *
     * @param action     The semantic camera action
     * @param payload    Position/delta/zoom data associated with the action
     * @param delta_time Time since last input in seconds
     * @return true if the action was handled (informational; does not block other manipulators)
     */
    virtual bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept = 0;

    /**
     * @brief Advance manipulator state by one frame (inertia, damping, autonomous motion).
     * @param delta_time Elapsed time in seconds since last frame
     */
    virtual void onUpdate(double delta_time) noexcept = 0;

    /**
     * @brief Set the camera this manipulator controls.
     * @param camera Shared pointer to the camera; may be nullptr to detach
     */
    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;

    /**
     * @brief Notify the manipulator that the viewport was resized (e.g. window or canvas size changed).
     * @param width_px  New viewport width in pixels (>= 1)
     * @param height_px New viewport height in pixels (>= 1)
     */
    virtual void onResize(float width_px, float height_px) noexcept = 0;

    /** @brief Reset all interaction state (velocities, inertia, drag tracking). */
    virtual void resetState() noexcept = 0;

    /** @return true if this manipulator is enabled and will process actions */
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;

    /**
     * @brief Enable or disable this manipulator.
     * @param enabled false = manipulator ignores all actions and onUpdate calls
     */
    virtual void setEnabled(bool enabled) noexcept = 0;
};

/**
 * @brief Deprecated alias for @ref ICameraManipulator.
 * @deprecated Use @ref ICameraManipulator. Will be removed in a future release.
 */
using ICameraBehavior [[deprecated("Use ICameraManipulator instead")]] = ICameraManipulator;

}  // namespace vne::interaction
