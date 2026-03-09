/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/types.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>

namespace vne::interaction {

using namespace vne::math;

namespace {
// --- Epsilon and numeric ---
constexpr float kEpsilon = 1e-6f;
constexpr float kMinViewportSize = 1.0f;
constexpr float kMinRadiusFallback = 1.0f;

// --- Fit-to-AABB ---
/// Distance from AABB center as multiple of radius so the box fits in view
constexpr float kFitToAabbDistanceFactor = 2.5f;

// --- Angle limits (degrees) ---
/// Pitch clamp to avoid gimbal flip at poles
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;

// --- FOV limits (degrees) ---
constexpr float kFovMinDeg = 5.0f;
constexpr float kFovMaxDeg = 120.0f;

// --- Scene scale clamp ---
constexpr float kSceneScaleMin = 1e-4f;
constexpr float kSceneScaleMax = 1e4f;

// --- Key codes (e.g. GLFW) ---
constexpr int kKeyW = 87;
constexpr int kKeyA = 65;
constexpr int kKeyS = 83;
constexpr int kKeyD = 68;
constexpr int kKeyQ = 81;
constexpr int kKeyE = 69;
constexpr int kKeyLeftShift = 340;
constexpr int kKeyRightShift = 344;
constexpr int kKeyLeftCtrl = 341;
constexpr int kKeyRightCtrl = 345;

// --- Touch ---
/// Scale factor for touch pan vs mouse (degrees per pixel)
constexpr float kTouchPanSensitivityFactor = 0.5f;
}

void FlyManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncAnglesFromCamera();
}

void FlyManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(kMinViewportSize, width_px);
    viewport_height_ = std::max(kMinViewportSize, height_px);
}

void FlyManipulator::resetState() noexcept {
    w_ = false;
    a_ = false;
    s_ = false;
    d_ = false;
    q_ = false;
    e_ = false;
    sprint_ = false;
    slow_ = false;
    looking_ = false;
}

void FlyManipulator::fitToAABB(const Vec3f& min_world, const Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const Vec3f center = (min_world + max_world) * 0.5f;
    float radius = (max_world - min_world).length() * 0.5f;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }
    const Vec3f f = front();
    camera_->setPosition(center - f * (radius * kFitToAabbDistanceFactor));
    camera_->setTarget(center);
    camera_->updateMatrices();
}

Vec3f FlyManipulator::front() const noexcept {
    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = std::cos(pitch_rad);
    Vec3f f(std::sin(yaw_rad) * cp, std::sin(pitch_rad), -std::cos(yaw_rad) * cp);
    const float len = f.lengthSquared();
    return (len < kEpsilon) ? Vec3f(0.0f, 0.0f, -1.0f) : (f / len);
}

Vec3f FlyManipulator::upAxis() const noexcept {
    if (!camera_) {
        return Vec3f(0.0f, 1.0f, 0.0f);
    }
    Vec3f up = camera_->getUp();
    return up.isZero(kEpsilon) ? Vec3f(0.0f, 1.0f, 0.0f) : up.normalized();
}

Vec3f FlyManipulator::right(const Vec3f& front_vec) const noexcept {
    Vec3f r = upAxis().cross(front_vec);
    return r.isZero(kEpsilon) ? Vec3f(1.0f, 0.0f, 0.0f) : r.normalized();
}

void FlyManipulator::syncAnglesFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    Vec3f f = camera_->getTarget() - camera_->getPosition();
    const float len = f.length();
    if (len < kEpsilon || f.isZero(kEpsilon)) {
        yaw_deg_ = 0.0f;
        pitch_deg_ = 0.0f;
        return;
    }
    f /= len;
    pitch_deg_ = vne::math::radToDeg(vne::math::asin(vne::math::clamp(f.y(), -1.0f, 1.0f)));
    yaw_deg_ = vne::math::radToDeg(vne::math::atan2(f.x(), -f.z()));
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
    if (w_) {
        move += f;
    }
    if (s_) {
        move -= f;
    }
    if (d_) {
        move += r;
    }
    if (a_) {
        move -= r;
    }
    if (e_) {
        move += up;
    }
    if (q_) {
        move -= up;
    }

    if (move.length() <= kEpsilon) {
        return;
    }

    float speed = move_speed_;
    if (sprint_) {
        speed *= sprint_mult_;
    } else if (slow_) {
        speed *= slow_mult_;
    }

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
    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
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
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_step_or_factor, kSceneScaleMin, kSceneScaleMax);
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
                persp->setFieldOfView(vne::math::clamp(fov * zoom_step_or_factor, kFovMinDeg, kFovMaxDeg));
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
    if (key == kKeyW) {
        w_ = pressed;
    } else if (key == kKeyS) {
        s_ = pressed;
    } else if (key == kKeyA) {
        a_ = pressed;
    } else if (key == kKeyD) {
        d_ = pressed;
    } else if (key == kKeyQ) {
        q_ = pressed;
    } else if (key == kKeyE) {
        e_ = pressed;
    } else if (key == kKeyLeftShift || key == kKeyRightShift) {
        sprint_ = pressed;
    } else if (key == kKeyLeftCtrl || key == kKeyRightCtrl) {
        slow_ = pressed;
    }
}

void FlyManipulator::handleTouchPan(const TouchPan& pan, double) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    yaw_deg_ += pan.delta_x_px * mouse_sensitivity_ * kTouchPanSensitivityFactor;
    pitch_deg_ += -pan.delta_y_px * mouse_sensitivity_ * kTouchPanSensitivityFactor;
    pitch_deg_ = std::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
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
