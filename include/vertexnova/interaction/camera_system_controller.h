#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_controller.h"
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/detail/camera_input_adapter.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>
#include <string>

namespace vne::interaction {

/**
 * @brief Controller that owns the active camera and manipulator, implements ICameraController.
 * Forwards input to the manipulator and drives updates. When the manipulator or camera
 * is set, the controller assigns the camera to the manipulator.
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API CameraSystemController : public ICameraController {
   public:
    CameraSystemController() = default;
    ~CameraSystemController() noexcept override = default;

    // ICameraController
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setEnabled(bool enabled) noexcept override;
    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void update(double delta_time) noexcept override;
    void reset() noexcept override;
    [[nodiscard]] std::shared_ptr<vne::scene::ICamera> getCamera() const noexcept override { return camera_; }
    [[nodiscard]] const std::string& getName() const noexcept override { return name_; }
    void setName(const std::string& name) noexcept override { name_ = name; }

    void setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept;
    [[nodiscard]] std::shared_ptr<ICameraManipulator> getManipulator() const noexcept { return manipulator_; }

    void setViewportSize(float width_px, float height_px) noexcept;
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept;
    void handleMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept;

   private:
    void assignCameraToManipulator() noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    std::shared_ptr<ICameraManipulator> manipulator_;
    CameraInputAdapter input_adapter_;
    bool enabled_ = true;
    std::string name_ = "CameraSystemController";
};

}  // namespace vne::interaction
