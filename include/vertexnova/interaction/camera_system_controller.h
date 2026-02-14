#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_controller.h"
#include <memory>
#include <string>

namespace vne::interaction {

class CameraSystemController final : public vne::scene::ICameraController {
public:
    explicit CameraSystemController(CameraManipulatorType type = CameraManipulatorType::eOrbitArcball) noexcept;
    ~CameraSystemController() noexcept override = default;

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setEnabled(bool enabled) noexcept override;
    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void update(double delta_time) noexcept override;
    void reset() noexcept override;

    void setViewportSize(float width_px, float height_px) noexcept;

    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept;
    void handleMouseScroll(float scroll_x, float scroll_y, double delta_time) noexcept;
    void handleMouseScrollEx(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept;

    void setManipulator(CameraManipulatorType type) noexcept;
    [[nodiscard]] CameraManipulatorType getManipulatorType() const noexcept { return type_; }
    [[nodiscard]] ICameraManipulator* getManipulator() noexcept { return manipulator_.get(); }
    [[nodiscard]] const ICameraManipulator* getManipulator() const noexcept { return manipulator_.get(); }

    [[nodiscard]] std::shared_ptr<vne::scene::ICamera> getCamera() const noexcept override;
    [[nodiscard]] const std::string& getName() const noexcept override { return name_; }
    void setName(const std::string& name) noexcept override { name_ = name; }

private:
    void recreateManipulator() noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    CameraManipulatorType type_;
    std::unique_ptr<ICameraManipulator> manipulator_;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    std::string name_ = "CameraSystemController";
};

}  // namespace vne::interaction
