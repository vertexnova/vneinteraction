#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_controller.h
 * @brief Interface for camera controllers (implemented by vneinteraction; uses ICamera from vnescene).
 */

#include "vertexnova/interaction/export.h"
#include <vertexnova/scene/camera/camera.h>

#include <memory>
#include <string>

namespace vne::interaction {

/**
 * @class ICameraController
 * @brief Interface for objects that control an ICamera (input, update, enable/disable).
 *
 * Implementations (e.g. CameraSystemController) attach to a camera and drive it from
 * input or other logic. Uses vne::scene::ICamera from the vnescene library.
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API ICameraController {
   public:
    virtual ~ICameraController() noexcept = default;

    /**
     * @brief Set the camera to control.
     * @param camera Shared pointer to the camera (may be nullptr)
     */
    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;

    /**
     * @brief Enable or disable this controller.
     * @param enabled true to enable, false to disable
     */
    virtual void setEnabled(bool enabled) noexcept = 0;

    /**
     * @brief Check if this controller is enabled.
     * @return true if enabled
     */
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;

    /**
     * @brief Update controller state (call each frame).
     * @param delta_time Time since last update in seconds
     */
    virtual void update(double delta_time) noexcept = 0;

    /** Reset controller state. */
    virtual void reset() noexcept = 0;

    /**
     * @brief Get the controlled camera.
     * @return Shared pointer to the camera, or nullptr
     */
    [[nodiscard]] virtual std::shared_ptr<vne::scene::ICamera> getCamera() const noexcept = 0;

    /**
     * @brief Get the controller name.
     * @return Reference to the name string
     */
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;

    /**
     * @brief Set the controller name.
     * @param name New name
     */
    virtual void setName(const std::string& name) noexcept = 0;
};

}  // namespace vne::interaction
