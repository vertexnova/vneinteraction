#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_controller.h
 * @brief ICameraController — polymorphic base for high-level viewport camera facades.
 *
 * Concrete implementations: @ref Inspect3DController, @ref Navigation3DController,
 * @ref Ortho2DController, @ref FollowController. Each composes @ref CameraRig, @ref InputMapper,
 * and camera/viewport lifecycle; this interface exposes only the uniform setup and per-frame API.
 *
 * @par When to use
 * Store `std::unique_ptr<ICameraController>` when the active controller type is chosen at runtime
 * (editor modes, tool switching). Otherwise include the concrete controller header directly.
 */

#include "vertexnova/interaction/export.h"

#include <memory>

namespace vne::events {
class Event;
}

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Virtual interface for `setCamera` / viewport / frame tick / event feed.
 *
 * Matches the common workflow: attach a \c vne::scene::ICamera, call @ref onResize when the drawable
 * size changes, forward window events to @ref onEvent, and call @ref onUpdate each frame for inertia
 * and autonomous manipulators (e.g. follow).
 *
 * @threadsafe Implementations are not thread-safe unless documented otherwise.
 */
class VNE_INTERACTION_API ICameraController {
   public:
    virtual ~ICameraController() = default;

    /**
     * @brief Attach or detach the controlled camera.
     * @param camera Shared camera; @c nullptr detaches.
     */
    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;

    /**
     * @brief Notify viewport size in pixels (affects zoom-at-cursor, trackball projection, etc.).
     * @param width_px  Drawable width (pixels).
     * @param height_px Drawable height (pixels).
     */
    virtual void onResize(float width_px, float height_px) noexcept = 0;

    /**
     * @brief Advance simulation time (inertia, damping, follow smoothing).
     * @param delta_time Elapsed seconds since last call.
     */
    virtual void onUpdate(double delta_time) noexcept = 0;

    /**
     * @brief Feed one UI/window event (mouse, key, touch).
     * @param event       vneevents event reference.
     * @param delta_time  Optional per-event time step; default @c 0.0 if unknown.
     */
    virtual void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept = 0;
};

}  // namespace vne::interaction
