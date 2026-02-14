/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fps_fly_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include <vertexnova/math/core/core.h>
#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne::math;

namespace {
constexpr float kEpsilon = 1e-6f;
float radToDeg(float rad) noexcept { return rad * 57.295779513082320876f; }
}  // namespace

void FpsFlyManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncAnglesFromCamera();
}

void FpsFlyManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
}

Vec3f FpsFlyManipulator::front() const noexcept {
    float yaw_rad = degToRad(yaw_deg_);
    float pitch_rad = degToRad(pitch_deg_);
    float cp = std::cos(pitch_rad);
    Vec3f f(std::sin(yaw_rad) * cp, std::sin(pitch_rad), -std::cos(yaw_rad) * cp);
    float len = f.length();
    return (len < kEpsilon) ? Vec3f(0.0f, 0.0f, -1.0f) : (f / len);
}

Vec3f FpsFlyManipulator::right(const Vec3f& front) const noexcept {
    Vec3f up(0.0f, 1.0f, 0.0f);
    Vec3f r = front.cross(up);
    float len = r.length();
    return (len < kEpsilon) ? Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

void FpsFlyManipulator::syncAnglesFromCamera() noexcept {
    if (!camera_) return;
    Vec3f front = camera_->getTarget() - camera_->getPosition();
    float len = front.length();
    if (len < kEpsilon) {
        yaw_deg_ = 0.0f;
        pitch_deg_ = 0.0f;
        return;
    }
    front = front / len;
    pitch_deg_ = radToDeg(std::asin(std::clamp(front.y(), -1.0f, 1.0f)));
    yaw_deg_ = radToDeg(std::atan2(front.x(), -front.z()));
}

void FpsFlyManipulator::applyAnglesToCamera() noexcept {
    if (!camera_) return;
    Vec3f eye = camera_->getPosition();
    Vec3f f = front();
    camera_->setTarget(eye + f);
    camera_->setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
}

void FpsFlyManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    float dt = static_cast<float>(delta_time);
    if (dt <= 0.0f) return;
    Vec3f f = front();
    Vec3f r = right(f);
    Vec3f move(0.0f, 0.0f, 0.0f);
    if (w_) move = move + f;
    if (s_) move = move - f;
    if (d_) move = move + r;
    if (a_) move = move - r;
    if (e_) move = move + Vec3f(0.0f, 1.0f, 0.0f);
    if (q_) move = move - Vec3f(0.0f, 1.0f, 0.0f);
    if (move.length() > kEpsilon) {
        move = move.normalized() * (move_speed_ * dt);
        camera_->setPosition(camera_->getPosition() + move);
        camera_->setTarget(camera_->getTarget() + move);
        camera_->updateMatrices();
    }
}

void FpsFlyManipulator::handleMouseMove(float, float, float delta_x, float delta_y, double) noexcept {
    if (!enabled_ || !camera_ || !looking_) return;
    yaw_deg_ += delta_x * mouse_sensitivity_;
    pitch_deg_ += -delta_y * mouse_sensitivity_;
    pitch_deg_ = std::clamp(pitch_deg_, -89.0f, 89.0f);
    applyAnglesToCamera();
}

void FpsFlyManipulator::handleMouseButton(int button, bool pressed, float, float, double) noexcept {
    if (!enabled_ || !camera_) return;
    if (button == 1) looking_ = pressed;
}

void FpsFlyManipulator::handleMouseScroll(float, float scroll_y, float, float, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) return;
    float dist = (scroll_y > 0.0f) ? move_speed_ * 0.5f : -move_speed_ * 0.5f;
    Vec3f f = front();
    camera_->setPosition(camera_->getPosition() + f * dist);
    camera_->setTarget(camera_->getTarget() + f * dist);
    camera_->updateMatrices();
}

void FpsFlyManipulator::handleKeyboard(int key, bool pressed, double) noexcept {
    if (!enabled_) return;
    if (key == 87) w_ = pressed;   // W
    else if (key == 83) s_ = pressed;  // S
    else if (key == 65) a_ = pressed;  // A
    else if (key == 68) d_ = pressed;  // D
    else if (key == 81) q_ = pressed;  // Q
    else if (key == 69) e_ = pressed;  // E
}

void FpsFlyManipulator::handleTouchPan(const TouchPan& pan, double) noexcept {
    if (!enabled_ || !camera_ || !looking_) return;
    yaw_deg_ += pan.delta_x_px * mouse_sensitivity_ * 0.5f;
    pitch_deg_ += -pan.delta_y_px * mouse_sensitivity_ * 0.5f;
    pitch_deg_ = std::clamp(pitch_deg_, -89.0f, 89.0f);
    applyAnglesToCamera();
}

void FpsFlyManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) return;
    float dist = (pinch.scale > 1.0f) ? -move_speed_ * 0.3f : move_speed_ * 0.3f;
    Vec3f f = front();
    camera_->setPosition(camera_->getPosition() + f * dist);
    camera_->setTarget(camera_->getTarget() + f * dist);
    camera_->updateMatrices();
}

}  // namespace vne::interaction
