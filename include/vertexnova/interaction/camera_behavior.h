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
 *
 * Distinct from ICameraManipulator (orbit, FPS, fly), which are driven by
 * direct user input. A FollowManipulator implements both for compatibility.
 *
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API ICameraBehavior {
   public:
    virtual ~ICameraBehavior() noexcept = default;

    /**
     * @brief Set the camera to drive.
     * @param camera Shared pointer to the camera (may be nullptr)
     */
    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;

    /**
     * @brief Enable or disable this behavior.
     * @param enabled true to enable, false to disable
     */
    virtual void setEnabled(bool enabled) noexcept = 0;

    /**
     * @brief Check if this behavior is enabled.
     * @return true if enabled
     */
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;

    /**
     * @brief Update the camera based on behavior logic (e.g. target tracking).
     * @param delta_time Time since last update in seconds
     */
    virtual void update(double delta_time) noexcept = 0;

    /** @brief Reset behavior state. */
    virtual void resetState() noexcept = 0;
};

}  // namespace vne::interaction
