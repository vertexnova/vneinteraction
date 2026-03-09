/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/detail/free_camera_base.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;
constexpr float kFovMinDeg = 5.0f;
constexpr float kFovMaxDeg = 120.0f;
constexpr float kSceneScaleMin = 1e-4f;
constexpr float kSceneScaleMax = 1e4f;
constexpr float kTouchPanSensitivityFactor = 0.5f;
constexpr int kKeyW = 87;
constexpr int kKeyS = 83;
constexpr int kKeyA = 65;
constexpr int kKeyD = 68;
constexpr int kKeyQ = 81;
constexpr int kKeyE = 69;
constexpr int kKeyLeftShift = 340;
constexpr int kKeyRightShift = 344;
constexpr int kKeyLeftCtrl = 341;
constexpr int kKeyRightCtrl = 345;
}  // namespace

void FreeCameraBase::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncAnglesFromCamera();
}

vne::math::Vec3f FreeCameraBase::front() const noexcept {
    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = vne::math::cos(pitch_rad);
    vne::math::Vec3f f(vne::math::sin(yaw_rad) * cp, vne::math::sin(pitch_rad), -vne::math::cos(yaw_rad) * cp);
    const float len = f.length();
    return (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (f / len);
}

vne::math::Vec3f FreeCameraBase::right(const vne::math::Vec3f& front_vec) const noexcept {
    vne::math::Vec3f r = upVector().cross(front_vec);
    const float len = r.length();
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

void FreeCameraBase::syncAnglesFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    vne::math::Vec3f f = camera_->getTarget() - camera_->getPosition();
    const float len = f.length();
    if (len < kEpsilon) {
        yaw_deg_ = 0.0f;
        pitch_deg_ = 0.0f;
        return;
    }
    f /= len;
    pitch_deg_ = vne::math::radToDeg(vne::math::asin(vne::math::clamp(f.y(), -1.0f, 1.0f)));
    yaw_deg_ = vne::math::radToDeg(vne::math::atan2(f.x(), -f.z()));
}

void FreeCameraBase::applyAnglesToCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f eye = camera_->getPosition();
    camera_->setTarget(eye + front());
    camera_->setUp(upVector());
    camera_->updateMatrices();
}

void FreeCameraBase::applyZoom(float zoom_step_or_factor) noexcept {
    if (!camera_) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_step_or_factor, kSceneScaleMin, kSceneScaleMax);
            return;
        case ZoomMethod::eDollyToCoi: {
            const vne::math::Vec3f f = front();
            camera_->setPosition(camera_->getPosition() + f * zoom_step_or_factor);
            camera_->setTarget(camera_->getTarget() + f * zoom_step_or_factor);
            camera_->updateMatrices();
            return;
        }
        case ZoomMethod::eChangeFov: {
            auto persp = perspCamera();
            if (persp) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(vne::math::clamp(fov * zoom_step_or_factor, kFovMinDeg, kFovMaxDeg));
                persp->updateMatrices();
            }
            return;
        }
    }
}

void FreeCameraBase::applyCommand(CameraActionType action, const CameraCommandPayload& payload, double) noexcept {
    if (!enabled_) {
        return;
    }
    switch (action) {
        case CameraActionType::BeginLook:
            input_state_.looking = true;
            break;
        case CameraActionType::EndLook:
            input_state_.looking = false;
            break;
        case CameraActionType::LookDelta:
            if (camera_ && input_state_.looking) {
                yaw_deg_ += payload.delta_x_px * mouse_sensitivity_;
                pitch_deg_ += -payload.delta_y_px * mouse_sensitivity_;
                pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
                applyAnglesToCamera();
            }
            break;
        case CameraActionType::MoveForward:
            input_state_.move_forward = payload.pressed;
            break;
        case CameraActionType::MoveBackward:
            input_state_.move_backward = payload.pressed;
            break;
        case CameraActionType::MoveLeft:
            input_state_.move_left = payload.pressed;
            break;
        case CameraActionType::MoveRight:
            input_state_.move_right = payload.pressed;
            break;
        case CameraActionType::MoveUp:
            input_state_.move_up = payload.pressed;
            break;
        case CameraActionType::MoveDown:
            input_state_.move_down = payload.pressed;
            break;
        case CameraActionType::SprintModifier:
            input_state_.sprint = payload.pressed;
            break;
        case CameraActionType::SlowModifier:
            input_state_.slow = payload.pressed;
            break;
        case CameraActionType::ZoomAtCursor:
            if (camera_ && payload.zoom_factor > 0.0f) {
                if (zoom_method_ == ZoomMethod::eDollyToCoi) {
                    applyZoom((payload.zoom_factor < 1.0f) ? move_speed_ * zoom_speed_ : -move_speed_ * zoom_speed_);
                } else {
                    applyZoom((payload.zoom_factor < 1.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_);
                }
            }
            break;
        default:
            break;
    }
}

void FreeCameraBase::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    if (dt <= 0.0f) {
        return;
    }
    const vne::math::Vec3f f = front();
    const vne::math::Vec3f r = right(f);
    const vne::math::Vec3f up = upVector();
    vne::math::Vec3f move(0.0f, 0.0f, 0.0f);
    if (input_state_.move_forward) {
        move += f;
    }
    if (input_state_.move_backward) {
        move -= f;
    }
    if (input_state_.move_right) {
        move += r;
    }
    if (input_state_.move_left) {
        move -= r;
    }
    if (input_state_.move_up) {
        move += up;
    }
    if (input_state_.move_down) {
        move -= up;
    }
    if (move.length() <= kEpsilon) {
        return;
    }
    float speed = move_speed_;
    if (input_state_.sprint) {
        speed *= sprint_mult_;
    } else if (input_state_.slow) {
        speed *= slow_mult_;
    }
    move = move.normalized() * (speed * dt);
    camera_->setPosition(camera_->getPosition() + move);
    camera_->setTarget(camera_->getTarget() + move);
    camera_->updateMatrices();
}

void FreeCameraBase::handleMouseMove(float, float, float delta_x, float delta_y, double) noexcept {
    if (!enabled_ || !camera_ || !input_state_.looking) {
        return;
    }
    yaw_deg_ += delta_x * mouse_sensitivity_;
    pitch_deg_ += -delta_y * mouse_sensitivity_;
    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
    applyAnglesToCamera();
}

void FreeCameraBase::handleMouseButton(int button, bool pressed, float, float, double) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (button == static_cast<int>(MouseButton::eRight)) {
        input_state_.looking = pressed;
    }
}

void FreeCameraBase::handleMouseScroll(float, float scroll_y, float, float, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    if (zoom_method_ == ZoomMethod::eDollyToCoi) {
        applyZoom((scroll_y > 0.0f) ? move_speed_ * zoom_speed_ : -move_speed_ * zoom_speed_);
    } else {
        applyZoom((scroll_y > 0.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_);
    }
}

void FreeCameraBase::handleKeyboard(int key, bool pressed, double) noexcept {
    if (!enabled_) {
        return;
    }
    if (key == kKeyW) {
        input_state_.move_forward = pressed;
    } else if (key == kKeyS) {
        input_state_.move_backward = pressed;
    } else if (key == kKeyA) {
        input_state_.move_left = pressed;
    } else if (key == kKeyD) {
        input_state_.move_right = pressed;
    } else if (key == kKeyQ) {
        input_state_.move_down = pressed;
    } else if (key == kKeyE) {
        input_state_.move_up = pressed;
    } else if (key == kKeyLeftShift || key == kKeyRightShift) {
        input_state_.sprint = pressed;
    } else if (key == kKeyLeftCtrl || key == kKeyRightCtrl) {
        input_state_.slow = pressed;
    }
}

void FreeCameraBase::handleTouchPan(const TouchPan& pan, double) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    yaw_deg_ += pan.delta_x_px * mouse_sensitivity_ * kTouchPanSensitivityFactor;
    pitch_deg_ += -pan.delta_y_px * mouse_sensitivity_ * kTouchPanSensitivityFactor;
    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
    applyAnglesToCamera();
}

void FreeCameraBase::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) {
        return;
    }
    if (zoom_method_ == ZoomMethod::eDollyToCoi) {
        applyZoom((pinch.scale > 1.0f) ? (-move_speed_ * zoom_speed_) : (move_speed_ * zoom_speed_));
    } else {
        applyZoom(1.0f / pinch.scale);
    }
}

}  // namespace vne::interaction
