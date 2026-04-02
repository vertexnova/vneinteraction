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
 * @brief ICameraController — common virtual interface for high-level camera controllers.
 *
 * Concrete types: @ref Inspect3DController, @ref Navigation3DController, @ref Ortho2DController,
 * @ref FollowController. Use when you need a single pointer type (e.g. `std::unique_ptr<ICameraController>`).
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
 * @brief Polymorphic base for bundled rig + input + camera lifecycle.
 *
 * @threadsafe Implementations are not thread-safe unless documented otherwise.
 */
class VNE_INTERACTION_API ICameraController {
   public:
    virtual ~ICameraController() = default;

    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;
    virtual void onResize(float width_px, float height_px) noexcept = 0;
    virtual void onUpdate(double delta_time) noexcept = 0;
    virtual void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept = 0;
};

}  // namespace vne::interaction
