/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/interaction/behavior_math.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Anonymous constants (mirrors free_camera_base.cpp)
// ---------------------------------------------------------------------------
namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.free_look");
constexpr float kEpsilon = 1e-6f;
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbDistFactor = 2.5f;  // fallback multiplier for non-perspective cameras
constexpr float kFitToAabbMargin = 1.1f;      // 10 % breathing room added to FOV-derived distance
}  // namespace

// ---------------------------------------------------------------------------
// Up-vector policy
// ---------------------------------------------------------------------------

vne::math::Vec3f FreeLookBehavior::upVector() const noexcept {
    if (mode_ == FreeLookMode::eFps) {
        // FPS: fixed world up
        return world_up_;
    }
    // Fly: use actual camera up (may have roll)
    if (!camera_) {
        return vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    }
    vne::math::Vec3f up = camera_->getUp();
    return up.isZero(kEpsilon) ? vne::math::Vec3f(0.0f, 1.0f, 0.0f) : up.normalized();
}

// ---------------------------------------------------------------------------
// ICameraBehavior: setCamera / onResize
// ---------------------------------------------------------------------------

void FreeLookBehavior::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraBehaviorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "FreeLookBehavior: camera detached (null camera)";
    }
    syncAnglesFromCamera();
}

void FreeLookBehavior::onResize(float width_px, float height_px) noexcept {
    CameraBehaviorBase::onResize(width_px, height_px);
}

// ---------------------------------------------------------------------------
// Movement math (copied from FreeCameraBase)
// ---------------------------------------------------------------------------

vne::math::Vec3f FreeLookBehavior::front() const noexcept {
    const vne::math::Vec3f up = upVector();
    vne::math::Vec3f ref_fwd, ref_right;
    buildReferenceFrame(up, ref_fwd, ref_right);

    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = vne::math::cos(pitch_rad);
    vne::math::Vec3f f =
        (ref_fwd * vne::math::cos(yaw_rad) + ref_right * vne::math::sin(yaw_rad)) * cp + up * vne::math::sin(pitch_rad);
    const float len = f.length();
    return (len < kEpsilon) ? ref_fwd : (f / len);
}

vne::math::Vec3f FreeLookBehavior::right(const vne::math::Vec3f& front_vec) const noexcept {
    vne::math::Vec3f r = front_vec.cross(upVector());
    const float len = r.length();
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

void FreeLookBehavior::syncAnglesFromCamera() noexcept {
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

    const vne::math::Vec3f up = upVector();
    const float up_comp = vne::math::clamp(f.dot(up), -1.0f, 1.0f);
    pitch_deg_ = vne::math::radToDeg(vne::math::asin(up_comp));

    const vne::math::Vec3f horiz = f - up * up_comp;
    const float horiz_len = horiz.length();
    if (horiz_len < kEpsilon) {
        yaw_deg_ = 0.0f;
        return;
    }
    const vne::math::Vec3f horiz_n = horiz / horiz_len;
    vne::math::Vec3f ref_fwd, ref_right;
    buildReferenceFrame(up, ref_fwd, ref_right);
    yaw_deg_ = vne::math::radToDeg(vne::math::atan2(horiz_n.dot(ref_right), horiz_n.dot(ref_fwd)));
}

void FreeLookBehavior::applyAnglesToCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f eye = camera_->getPosition();
    const vne::math::Vec3f f = front();
    vne::math::Vec3f up;
    if (mode_ == FreeLookMode::eFly) {
        const vne::math::Vec3f up_hint = upVector();
        vne::math::Vec3f r = f.cross(up_hint);
        float r_len = r.length();
        if (r_len < kEpsilon) {
            r = f.cross(world_up_);
            r_len = r.length();
        }
        if (r_len < kEpsilon) {
            // Both up_hint and world_up_ are collinear with f (e.g. looking straight up in fly mode).
            // Derive a deterministic right from the yaw-based reference frame instead of stale camera up.
            vne::math::Vec3f ref_fwd, ref_right;
            buildReferenceFrame(world_up_, ref_fwd, ref_right);
            r = ref_right;
        }
        up = r.cross(f).normalized();
    } else {
        up = upVector();
    }
    setCameraLookAt(camera_, eye, eye + f, up);
}

void FreeLookBehavior::onZoomDolly(float factor, float mx, float my) noexcept {
    // Ortho: delegate to base cursor-anchored zoom.
    if (orthoCamera()) {
        CameraBehaviorBase::onZoomDolly(factor, mx, my);
        return;
    }
    // Perspective (or unknown): move camera + target along front.
    if (!camera_) {
        return;
    }
    const float step = (factor < 1.0f) ? (move_speed_ * zoom_speed_) : -(move_speed_ * zoom_speed_);
    const vne::math::Vec3f f = front();
    camera_->setPosition(camera_->getPosition() + f * step);
    camera_->setTarget(camera_->getTarget() + f * step);
    camera_->updateMatrices();
}

// ---------------------------------------------------------------------------
// setWorldUp
// ---------------------------------------------------------------------------

void FreeLookBehavior::setWorldUp(const vne::math::Vec3f& up) noexcept {
    if (up.length() > kEpsilon) {
        world_up_ = up.normalized();
    } else {
        VNE_LOG_WARN << "FreeLookBehavior: setWorldUp called with zero-length vector, ignoring";
    }
}

// ---------------------------------------------------------------------------
// getWorldUnitsPerPixel / fitToAABB
// ---------------------------------------------------------------------------

float FreeLookBehavior::getWorldUnitsPerPixel() const noexcept {
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * vne::math::tan(fov_y_rad * 0.5f) / viewport().height;
    }
    return 1.0f;
}

void FreeLookBehavior::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    float radius = (max_world - min_world).length() * 0.5f;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }
    const vne::math::Vec3f f = front();
    vne::math::Vec3f eye;
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float dist = (radius / vne::math::tan(fov_y_rad * 0.5f)) * kFitToAabbMargin;
        eye = center - f * dist;
    } else {
        eye = center - f * (radius * kFitToAabbDistFactor);
    }
    const vne::math::Vec3f up = (mode_ == FreeLookMode::eFps) ? world_up_ : upVector();
    setCameraLookAt(camera_, eye, center, up);
}

// ---------------------------------------------------------------------------
// resetState
// ---------------------------------------------------------------------------

void FreeLookBehavior::resetState() noexcept {
    input_state_ = FreeLookInputState{};
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void FreeLookBehavior::onUpdate(double delta_time) noexcept {
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

// ---------------------------------------------------------------------------
// onAction
// ---------------------------------------------------------------------------

bool FreeLookBehavior::onAction(CameraActionType action,
                                const CameraCommandPayload& payload,
                                double /*delta_time*/) noexcept {
    if (!enabled_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eBeginLook:
            input_state_.looking = true;
            return true;

        case CameraActionType::eEndLook:
            input_state_.looking = false;
            return true;

        case CameraActionType::eLookDelta:
            if (camera_ && input_state_.looking) {
                yaw_deg_ += payload.delta_x_px * mouse_sensitivity_;
                pitch_deg_ -= payload.delta_y_px * mouse_sensitivity_;
                if (mode_ == FreeLookMode::eFps) {
                    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
                }
                applyAnglesToCamera();
                return true;
            }
            return false;

        case CameraActionType::eMoveForward:
            input_state_.move_forward = payload.pressed;
            return true;

        case CameraActionType::eMoveBackward:
            input_state_.move_backward = payload.pressed;
            return true;

        case CameraActionType::eMoveLeft:
            input_state_.move_left = payload.pressed;
            return true;

        case CameraActionType::eMoveRight:
            input_state_.move_right = payload.pressed;
            return true;

        case CameraActionType::eMoveUp:
            input_state_.move_up = payload.pressed;
            return true;

        case CameraActionType::eMoveDown:
            input_state_.move_down = payload.pressed;
            return true;

        case CameraActionType::eSprintModifier:
            input_state_.sprint = payload.pressed;
            return true;

        case CameraActionType::eSlowModifier:
            input_state_.slow = payload.pressed;
            return true;

        case CameraActionType::eZoomAtCursor:
            if (!handle_zoom_) {
                return false;
            }
            if (camera_ && payload.zoom_factor > 0.0f && payload.zoom_factor != 1.0f) {
                dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px);
                return true;
            }
            return false;

        case CameraActionType::eResetView:
            resetState();
            return true;

        default:
            return false;
    }
}

}  // namespace vne::interaction
