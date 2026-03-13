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
 * @file camera_behavior.h
 * @brief ICameraBehavior — the composable camera behavior interface.
 *
 * Replaces ICameraManipulator. Multiple behaviors can be registered on a
 * CameraRig and each receives every CameraAction independently.
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Interface for a composable camera behavior.
 *
 * Each concrete behavior handles the subset of CameraActionType values relevant
 * to it and silently ignores the rest. Multiple behaviors can be active
 * simultaneously on a CameraRig — they each receive every action independently.
 *
 * Concrete implementations:
 *  - OrbitBehavior    — rotate/pan/zoom around a pivot (Euler or Quaternion mode)
 *  - FreeLookBehavior — WASD movement + mouse look (FPS or unconstrained Fly mode)
 *  - OrthoPanZoomBehavior — orthographic 2D pan + zoom
 *  - TrackBehavior    — autonomous smooth target following
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class VNE_INTERACTION_API ICameraBehavior {
   public:
    virtual ~ICameraBehavior() noexcept = default;

    /**
     * @brief Dispatch a camera action to this behavior.
     *
     * Called by CameraRig for every action in the stream. Behaviors silently
     * ignore actions they don't handle.
     *
     * @param action     The semantic camera action
     * @param payload    Position/delta/zoom data associated with the action
     * @param delta_time Time since last input in seconds
     * @return true if the action was handled (informational; does not block other behaviors)
     */
    virtual bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept = 0;

    /**
     * @brief Advance behavior state by one frame (inertia, damping, autonomous motion).
     * @param delta_time Elapsed time in seconds since last frame
     */
    virtual void onUpdate(double delta_time) noexcept = 0;

    /**
     * @brief Set the camera this behavior controls.
     * @param camera Shared pointer to the camera; may be nullptr to detach
     */
    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;

    /**
     * @brief Notify the behavior of the current viewport dimensions.
     * @param width_px  Viewport width in pixels (>= 1)
     * @param height_px Viewport height in pixels (>= 1)
     */
    virtual void setViewportSize(float width_px, float height_px) noexcept = 0;

    /** @brief Reset all interaction state (velocities, inertia, drag tracking). */
    virtual void resetState() noexcept = 0;

    /** @return true if this behavior is enabled and will process actions */
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;

    /**
     * @brief Enable or disable this behavior.
     * @param enabled false = behavior ignores all actions and onUpdate calls
     */
    virtual void setEnabled(bool enabled) noexcept = 0;
};

}  // namespace vne::interaction
