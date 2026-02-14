/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

namespace vne::interaction {

CameraSystemController::CameraSystemController(CameraManipulatorType type) noexcept : type_(type) {
    recreateManipulator();
}

void CameraSystemController::recreateManipulator() noexcept {
    manipulator_ = CameraManipulatorFactory::create(type_);
    if (manipulator_) {
        manipulator_->setEnabled(enabled_);
        manipulator_->setViewportSize(viewport_width_, viewport_height_);
        if (camera_) {
            manipulator_->setCamera(camera_);
        }
    }
}

void CameraSystemController::setManipulator(CameraManipulatorType type) noexcept {
    if (type_ == type) return;
    type_ = type;
    recreateManipulator();
}

void CameraSystemController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    if (manipulator_ && camera_) {
        manipulator_->setCamera(camera_);
    }
}

void CameraSystemController::setEnabled(bool enabled) noexcept {
    enabled_ = enabled;
    if (manipulator_) manipulator_->setEnabled(enabled);
}

void CameraSystemController::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = width_px;
    viewport_height_ = height_px;
    if (manipulator_) manipulator_->setViewportSize(width_px, height_px);
}

void CameraSystemController::update(double delta_time) noexcept {
    if (!enabled_ || !manipulator_) return;
    manipulator_->update(delta_time);
}

void CameraSystemController::reset() noexcept {
    recreateManipulator();
}

void CameraSystemController::handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    last_mouse_x_ = x;
    last_mouse_y_ = y;
    if (!enabled_ || !manipulator_) return;
    manipulator_->handleMouseMove(x, y, delta_x, delta_y, delta_time);
}

void CameraSystemController::handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
    last_mouse_x_ = x;
    last_mouse_y_ = y;
    if (!enabled_ || !manipulator_) return;
    manipulator_->handleMouseButton(button, pressed, x, y, delta_time);
}

void CameraSystemController::handleMouseScroll(float scroll_x, float scroll_y, double delta_time) noexcept {
    handleMouseScrollEx(scroll_x, scroll_y, last_mouse_x_, last_mouse_y_, delta_time);
}

void CameraSystemController::handleMouseScrollEx(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept {
    last_mouse_x_ = mouse_x;
    last_mouse_y_ = mouse_y;
    if (!enabled_ || !manipulator_) return;
    manipulator_->handleMouseScroll(scroll_x, scroll_y, mouse_x, mouse_y, delta_time);
}

void CameraSystemController::handleKeyboard(int key, bool pressed, double delta_time) noexcept {
    if (!enabled_ || !manipulator_) return;
    manipulator_->handleKeyboard(key, pressed, delta_time);
}

void CameraSystemController::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !manipulator_) return;
    manipulator_->handleTouchPan(pan, delta_time);
}

void CameraSystemController::handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept {
    if (!enabled_ || !manipulator_) return;
    manipulator_->handleTouchPinch(pinch, delta_time);
}

std::shared_ptr<vne::scene::ICamera> CameraSystemController::getCamera() const noexcept {
    return camera_;
}

}  // namespace vne::interaction
