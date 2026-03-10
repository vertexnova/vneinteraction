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

    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;
    virtual void setEnabled(bool enabled) noexcept = 0;
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;
    virtual void update(double delta_time) noexcept = 0;
    virtual void reset() noexcept = 0;

    [[nodiscard]] virtual std::shared_ptr<vne::scene::ICamera> getCamera() const noexcept = 0;
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;
    virtual void setName(const std::string& name) noexcept = 0;
};

}  // namespace vne::interaction
