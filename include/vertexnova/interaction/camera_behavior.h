#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_behavior.h
 * @brief Interface for camera behaviors (e.g. follow/tracking) as distinct from
 * direct user-driven manipulators. Behaviors drive the camera over time from
 * external state (target position, rig logic) rather than from raw input.
 */

#include "vertexnova/interaction/export.h"

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Interface for camera behaviors: follow target, tracking rig, etc.
 * Distinct from ICameraManipulator (orbit, FPS, fly), which are driven by
 * direct user input. A FollowManipulator implements both for compatibility.
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API ICameraBehavior {
   public:
    virtual ~ICameraBehavior() noexcept = default;

    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;
    virtual void update(double delta_time) noexcept = 0;
};

}  // namespace vne::interaction
