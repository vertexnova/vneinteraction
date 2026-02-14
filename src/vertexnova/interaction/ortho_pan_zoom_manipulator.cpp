/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include <vertexnova/math/core/core.h>
#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne::math;

namespace {
constexpr float kEpsilon = 1e-6f;
}

void OrthoPanZoomManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
}

void OrthoPanZoomManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
}

std::shared_ptr<vne::scene::OrthographicCamera> OrthoPanZoomManipulator::getOrtho() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
}

void OrthoPanZoomManipulator::pan(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    auto ortho = getOrtho();
    if (!ortho) return;
    Vec3f eye = ortho->getPosition();
    Vec3f target = ortho->getTarget();
    Vec3f front = target - eye;
    float len = front.length();
    if (len < kEpsilon) front = Vec3f(0.0f, 0.0f, -1.0f);
    else front = front / len;
    Vec3f up = ortho->getUp().normalized();
    Vec3f r = front.cross(up);
    len = r.length();
    r = (len < kEpsilon) ? Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
    float wppx = ortho->getWidth() / viewport_width_;
    float wppy = ortho->getHeight() / viewport_height_;
    Vec3f delta_world = r * (-delta_x_px * wppx) + up * (delta_y_px * wppy);
    ortho->setPosition(eye + delta_world);
    ortho->setTarget(target + delta_world);
    ortho->updateMatrices();
    if (delta_time > 0.0) pan_velocity_ = delta_world / static_cast<float>(delta_time);
}

void OrthoPanZoomManipulator::zoomToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    auto ortho = getOrtho();
    if (!ortho) return;
    zoom_factor = std::clamp(zoom_factor, 0.01f, 100.0f);
    float ndc_x = (2.0f * mouse_x_px / viewport_width_) - 1.0f;
    float ndc_y = 1.0f - (2.0f * mouse_y_px / viewport_height_);
    float half_w = ortho->getWidth() * 0.5f;
    float half_h = ortho->getHeight() * 0.5f;
    Vec3f eye = ortho->getPosition();
    Vec3f target = ortho->getTarget();
    Vec3f front = target - eye;
    float len = front.length();
    if (len < kEpsilon) front = Vec3f(0.0f, 0.0f, -1.0f);
    else front = front / len;
    Vec3f up = ortho->getUp().normalized();
    Vec3f r = front.cross(up);
    len = r.length();
    r = (len < kEpsilon) ? Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
    Vec3f world_at_cursor = target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
    float new_half_w = half_w * zoom_factor;
    float new_half_h = half_h * zoom_factor;
    Vec3f new_target = world_at_cursor - r * (ndc_x * new_half_w) - up * (ndc_y * new_half_h);
    Vec3f eye_offset = eye - target;
    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
}

void OrthoPanZoomManipulator::applyInertia(double delta_time) noexcept {
    if (!camera_ || delta_time <= 0.0) return;
    if (pan_velocity_.length() < 1e-4f) {
        pan_velocity_ = Vec3f(0.0f, 0.0f, 0.0f);
        return;
    }
    auto ortho = getOrtho();
    if (!ortho) return;
    float dt = static_cast<float>(delta_time);
    Vec3f delta = pan_velocity_ * dt;
    ortho->setPosition(ortho->getPosition() + delta);
    ortho->setTarget(ortho->getTarget() + delta);
    ortho->updateMatrices();
    pan_velocity_ = pan_velocity_ * std::exp(-pan_damping_ * dt);
}

void OrthoPanZoomManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    if (!panning_) applyInertia(delta_time);
}

void OrthoPanZoomManipulator::handleMouseMove(float, float, float delta_x, float delta_y, double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    if (panning_) pan(delta_x, delta_y, delta_time);
}

void OrthoPanZoomManipulator::handleMouseButton(int button, bool pressed, float, float, double) noexcept {
    if (!enabled_ || !camera_) return;
    if (button == 2 || button == 1) {
        panning_ = pressed;
        if (pressed) pan_velocity_ = Vec3f(0.0f, 0.0f, 0.0f);
    }
}

void OrthoPanZoomManipulator::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) return;
    float zoom_factor = (scroll_y > 0.0f) ? 0.9f : 1.1f;
    zoomToCursor(zoom_factor, mouse_x, mouse_y);
}

void OrthoPanZoomManipulator::handleKeyboard(int, bool, double) noexcept {}

void OrthoPanZoomManipulator::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    this->pan(pan.delta_x_px, pan.delta_y_px, delta_time);
}

void OrthoPanZoomManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) return;
    zoomToCursor(1.0f / pinch.scale, pinch.center_x_px, pinch.center_y_px);
}

}  // namespace vne::interaction
