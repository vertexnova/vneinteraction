/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_system_controller.h"

namespace vne::interaction {

void CameraSystemController::assignCameraToManipulator() noexcept {
    if (manipulator_ && camera_) {
        manipulator_->setCamera(camera_);
    }
}

void CameraSystemController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    assignCameraToManipulator();
}

void CameraSystemController::setEnabled(bool enabled) noexcept {
    enabled_ = enabled;
    if (manipulator_) {
        manipulator_->setEnabled(enabled);
    }
}

void CameraSystemController::update(double delta_time) noexcept {
    if (manipulator_) {
        manipulator_->update(delta_time);
    }
}

void CameraSystemController::reset() noexcept {
    if (manipulator_) {
        manipulator_->resetState();
    }
}

void CameraSystemController::setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept {
    manipulator_ = std::move(manipulator);
    input_adapter_.setManipulator(manipulator_);
    assignCameraToManipulator();
    if (manipulator_) {
        manipulator_->setEnabled(enabled_);
    }
}

void CameraSystemController::setViewportSize(float width_px, float height_px) noexcept {
    if (manipulator_) {
        manipulator_->setViewportSize(width_px, height_px);
    }
}

void CameraSystemController::handleMouseMove(
    float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    input_adapter_.feedMouseMove(x, y, delta_x, delta_y, delta_time);
}

void CameraSystemController::handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
    input_adapter_.feedMouseButton(button, pressed, x, y, delta_time);
}

void CameraSystemController::handleMouseScroll(
    float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept {
    input_adapter_.feedMouseScroll(scroll_x, scroll_y, mouse_x, mouse_y, delta_time);
}

void CameraSystemController::handleKeyboard(int key, bool pressed, double delta_time) noexcept {
    input_adapter_.feedKeyboard(key, pressed, delta_time);
}

void CameraSystemController::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    input_adapter_.feedTouchPan(pan, delta_time);
}

void CameraSystemController::handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept {
    input_adapter_.feedTouchPinch(pinch, delta_time);
}

}  // namespace vne::interaction
