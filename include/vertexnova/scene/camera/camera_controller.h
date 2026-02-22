#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_controller.h
 * @brief Interface for camera controllers (moved from vnescene; implemented by vneinteraction).
 */

#include "vertexnova/scene/camera/camera.h"
#include <memory>
#include <string>

namespace vne::scene {

/**
 * @class ICameraController
 * @brief Interface for objects that control an ICamera (input, update, enable/disable).
 *
 * Implementations (e.g. CameraSystemController in vneinteraction) attach to a camera
 * and drive it from input or other logic.
 */
class ICameraController {
public:
    virtual ~ICameraController() noexcept = default;

    virtual void setCamera(std::shared_ptr<ICamera> camera) noexcept = 0;
    virtual void setEnabled(bool enabled) noexcept = 0;
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;
    virtual void update(double delta_time) noexcept = 0;
    virtual void reset() noexcept = 0;

    [[nodiscard]] virtual std::shared_ptr<ICamera> getCamera() const noexcept = 0;
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;
    virtual void setName(const std::string& name) noexcept = 0;
};

}  // namespace vne::scene
