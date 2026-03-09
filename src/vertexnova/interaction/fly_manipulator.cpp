/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne::math;

namespace {
constexpr float kEpsilon = 1e-6f;
}

void FlyManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncAnglesFromCamera();
}

void FlyManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
}

void FlyManipulator::resetState() noexcept {
    w_ = a_ = s_ = d_ = q_ = e_ = false;
    sprint_ = slow_ = looking_ = false;
}

void FlyManipulator::fitToAABB(const Vec3f& min_world, const Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const Vec3f center = (min_world + max_world) * 0.5f;
    float radius = (max_world - min_world).length() * 0.5f;
    if (radius < kEpsilon) {
        radius = 1.0f;
    }
    const Vec3f f = front();
    camera_->setPosition(center - f * (radius * 2.5f));
    camera_->setTarget(center);
    camera_->updateMatrices();
}

Vec3f FlyManipulator::front() const noexcept {
    const float yaw_rad = degToRad(yaw_deg_);
    const float pitch_rad = degToRad(pitch_deg_);
    const float cp = std::cos(pitch_rad);
    Vec3f f(std::sin(yaw_rad) * cp, std::sin(pitch_rad), -std::cos(yaw_rad) * cp);
    const float len = f.length();
    return (len < kEpsilon) ? Vec3f(0.0f, 0.0f, -1.0f) : (f / len);
}

Vec3f FlyManipulator::upAxis() const noexcept {
    if (!camera_) {
        return Vec3f(0.0f, 1.0f, 0.0f);
    }
    Vec3f up = camera_->getUp();
    const float len = up.length();
    return (len < kEpsilon) ? Vec3f(0.0f, 1.0f, 0.0f) : (up / len);
}

Vec3f FlyManipulator::right(const Vec3f& front_vec) const noexcept {
    Vec3f r = upAxis().cross(front_vec);
    const float len = r.length();
    return (len < kEpsilon) ? Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

void FlyManipulator::syncAnglesFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    Vec3f f = camera_->getTarget() - camera_->getPosition();
    const float len = f.length();
    if (len < kEpsilon) {
        yaw_deg_ = 0.0f;
        pitch_deg_ = 0.0f;
        return;
    }
    f /= len;
    pitch_deg_ = radToDeg(std::asin(std::clamp(f.y(), -1.0f, 1.0f)));
    yaw_deg_ = radToDeg(std::atan2(f.x(), -f.z()));
}

void FlyManipulator::applyAnglesToCamera() noexcept {
    if (!camera_) {
        return;
    }
    const Vec3f eye = camera_->getPosition();
    camera_->setTarget(eye + front());
    camera_->updateMatrices();
}

void FlyManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    if (dt <= 0.0f) {
        return;
    }

    const Vec3f f = front();
    const Vec3f r = right(f);
    const Vec3f up = upAxis();

    Vec3f move(0.0f, 0.0f, 0.0f);
    if (w_)
        move += f;
    if (s_)
        move -= f;
    if (d_)
        move += r;
    if (a_)
        move -= r;
    if (e_)
        move += up;
    if (q_)
        move -= up;

    if (move.length() <= kEpsilon) {
        return;
    }

    float speed = move_speed_;
    if (sprint_)
        speed *= sprint_mult_;
    else if (slow_)
        speed *= slow_mult_;

    move = move.normalized() * (speed * dt);
    camera_->setPosition(camera_->getPosition() + move);
    camera_->setTarget(camera_->getTarget() + move);
    camera_->updateMatrices();
}

void FlyManipulator::handleMouseMove(float, float, float delta_x, float delta_y, double) noexcept {
    if (!enabled_ || !camera_ || !looking_) {
        return;
    }
    yaw_deg_ += delta_x * mouse_sensitivity_;
    pitch_deg_ += -delta_y * mouse_sensitivity_;
    pitch_deg_ = std::clamp(pitch_deg_, -89.0f, 89.0f);
    applyAnglesToCamera();
}

void FlyManipulator::handleMouseButton(int button, bool pressed, float, float, double) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (button == static_cast<int>(MouseButton::eRight)) {
        looking_ = pressed;
    }
}

void FlyManipulator::applyZoom(float zoom_step_or_factor) noexcept {
    if (!camera_) {
        return;
    }

    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = std::clamp(scene_scale_ * zoom_step_or_factor, 1e-4f, 1e4f);
            return;
        case ZoomMethod::eDollyToCoi: {
            const Vec3f f = front();
            camera_->setPosition(camera_->getPosition() + f * zoom_step_or_factor);
            camera_->setTarget(camera_->getTarget() + f * zoom_step_or_factor);
            camera_->updateMatrices();
            return;
        }
        case ZoomMethod::eChangeFov:
            if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(std::clamp(fov * zoom_step_or_factor, 5.0f, 120.0f));
                persp->updateMatrices();
            }
            return;
    }
}

void FlyManipulator::handleMouseScroll(float, float scroll_y, float, float, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    if (zoom_method_ == ZoomMethod::eDollyToCoi) {
        applyZoom((scroll_y > 0.0f) ? move_speed_ * zoom_speed_ : -move_speed_ * zoom_speed_);
    } else {
        applyZoom((scroll_y > 0.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_);
    }
}

void FlyManipulator::handleKeyboard(int key, bool pressed, double) noexcept {
    if (!enabled_) {
        return;
    }
    if (key == 87)
        w_ = pressed;
    else if (key == 83)
        s_ = pressed;
    else if (key == 65)
        a_ = pressed;
    else if (key == 68)
        d_ = pressed;
    else if (key == 81)
        q_ = pressed;
    else if (key == 69)
        e_ = pressed;
    else if (key == 340 || key == 344)
        sprint_ = pressed;
    else if (key == 341 || key == 345)
        slow_ = pressed;
}

void FlyManipulator::handleTouchPan(const TouchPan& pan, double) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    yaw_deg_ += pan.delta_x_px * mouse_sensitivity_ * 0.5f;
    pitch_deg_ += -pan.delta_y_px * mouse_sensitivity_ * 0.5f;
    pitch_deg_ = std::clamp(pitch_deg_, -89.0f, 89.0f);
    applyAnglesToCamera();
}

void FlyManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) {
        return;
    }
    if (zoom_method_ == ZoomMethod::eDollyToCoi) {
        applyZoom((pinch.scale > 1.0f) ? (-move_speed_ * zoom_speed_) : (move_speed_ * zoom_speed_));
    } else {
        applyZoom(1.0f / pinch.scale);
    }
}

float FlyManipulator::getWorldUnitsPerPixel() const noexcept {
    return 0.0f;
}

}  // namespace vne::interaction
