/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/free_look_manipulator.h"
#include "camera_math.h"
#include "view_math.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.free_look");
constexpr float kEpsilon = 1e-6f;
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbDistFactor = 2.5f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kPerspWorldUnitsScale = 2.0f;
constexpr float kHalf = 0.5f;

[[nodiscard]] vne::math::Vec3f normalizedWorldUp(const vne::math::Vec3f& w) noexcept {
    const float l = w.length();
    return (l >= kEpsilon) ? (w / l) : vne::math::Vec3f(0.0f, 1.0f, 0.0f);
}
}  // namespace

// ---------------------------------------------------------------------------
// Up-vector policy
// ---------------------------------------------------------------------------

vne::math::Vec3f FreeLookManipulator::upVector() const noexcept {
    if (mode_ == FreeLookMode::eFps) {
        return world_up_;
    }
    if (camera_) {
        vne::math::Vec3f up = camera_->getUpDir();
        const float ul = up.length();
        if (ul >= kEpsilon) {
            return up / ul;
        }
        up = camera_->getUp();
        return up.isZero(kEpsilon) ? vne::math::Vec3f{0.0f, 1.0f, 0.0f} : up.normalized();
    }
    return {0.0f, 1.0f, 0.0f};
}

// ---------------------------------------------------------------------------
// ICameraManipulator: setCamera / onResize
// ---------------------------------------------------------------------------

void FreeLookManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraManipulatorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "FreeLookManipulator: camera detached (null camera)";
        orientation_dirty_ = true;
        return;
    }
    syncOrientationFromCamera();
    orientation_dirty_ = false;
}

void FreeLookManipulator::onResize(float width_px, float height_px) noexcept {
    CameraManipulatorBase::onResize(width_px, height_px);
}

// ---------------------------------------------------------------------------
// Movement math
// ---------------------------------------------------------------------------

vne::math::Vec3f FreeLookManipulator::front() const noexcept {
    vne::math::Vec3f f = -orientation_.getZAxis();
    const float len = f.length();
    return (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (f / len);
}

vne::math::Vec3f FreeLookManipulator::right(const vne::math::Vec3f& front_vec) const noexcept {
    if (mode_ == FreeLookMode::eFly) {
        vne::math::Vec3f r = orientation_.getXAxis();
        const float len = r.length();
        return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
    }
    const vne::math::Vec3f wu = normalizedWorldUp(world_up_);
    vne::math::Vec3f r = front_vec.cross(wu);
    const float len = r.length();
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

vne::math::Vec3f FreeLookManipulator::orthoPanUp(const vne::math::Vec3f& view_dir,
                                                 const vne::math::Vec3f& vertical_hint) const noexcept {
    const vne::math::Vec3f f =
        view_dir.length() < kEpsilon ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : view_dir.normalized();
    vne::math::Vec3f u = camera_->getUp();
    u = u - f * u.dot(f);
    float len = u.length();
    if (len < kEpsilon) {
        const vne::math::Vec3f r = right(f);
        u = r.cross(f);
        len = u.length();
    }
    if (len < kEpsilon) {
        vne::math::Vec3f w = vertical_hint;
        u = w - f * w.dot(f);
        len = u.length();
    }
    if (len < kEpsilon) {
        vne::math::Vec3f ref_fwd, ref_right;
        buildReferenceFrame(vertical_hint, ref_fwd, ref_right);
        u = ref_fwd - f * ref_fwd.dot(f);
        len = u.length();
    }
    if (len < kEpsilon) {
        return {0.0f, 1.0f, 0.0f};
    }
    return u / len;
}

vne::math::Vec3f FreeLookManipulator::orthoPanUp(const vne::math::Vec3f& view_dir) const noexcept {
    return orthoPanUp(view_dir, upVector());
}

vne::math::Vec3f FreeLookManipulator::orthoPanUp(const vne::scene::OrthographicCamera& ortho,
                                                 const vne::math::Vec3f& vertical_hint) const noexcept {
    const vne::math::Vec3f view_raw = ortho.getTarget() - ortho.getPosition();
    const float view_len = view_raw.length();
    const vne::math::Vec3f view_dir = (view_len >= kEpsilon) ? (view_raw / view_len) : front();
    return orthoPanUp(view_dir, vertical_hint);
}

void FreeLookManipulator::syncOrientationFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    orientation_ = camera_->getOrientation().normalized();
}

void FreeLookManipulator::ensureAnglesSynced() noexcept {
    if (!orientation_dirty_ || !camera_) {
        return;
    }
    syncOrientationFromCamera();
    orientation_dirty_ = false;
}

void FreeLookManipulator::applyOrientationToCamera() noexcept {
    if (!camera_) {
        return;
    }
    camera_->setOrientationView(camera_->getPosition(), orientation_.normalized());
    camera_->updateMatrices();
}

void FreeLookManipulator::yawPitchFromOrientation(float& yaw_deg_out, float& pitch_deg_out) const noexcept {
    vne::math::Vec3f f = front();

    vne::math::Vec3f up_ref;
    if (mode_ == FreeLookMode::eFps) {
        up_ref = normalizedWorldUp(world_up_);
    } else {
        up_ref = orientation_.getYAxis();
        const float ul = up_ref.length();
        if (ul < kEpsilon) {
            up_ref = normalizedWorldUp(world_up_);
        } else {
            up_ref /= ul;
        }
    }

    const float up_comp = vne::math::clamp(f.dot(up_ref), -1.0f, 1.0f);
    pitch_deg_out = vne::math::radToDeg(vne::math::asin(up_comp));

    const vne::math::Vec3f horiz = f - up_ref * up_comp;
    const float horiz_len = horiz.length();
    if (horiz_len < kEpsilon) {
        yaw_deg_out = 0.0f;
        return;
    }
    const vne::math::Vec3f horiz_n = horiz / horiz_len;
    vne::math::Vec3f ref_fwd;
    vne::math::Vec3f ref_right;
    buildReferenceFrame(up_ref, ref_fwd, ref_right);
    yaw_deg_out = vne::math::radToDeg(vne::math::atan2(horiz_n.dot(ref_right), horiz_n.dot(ref_fwd)));
}

void FreeLookManipulator::clampFpsPitch() noexcept {
    if (mode_ != FreeLookMode::eFps) {
        return;
    }
    const vne::math::Vec3f wu = normalizedWorldUp(world_up_);
    vne::math::Vec3f f = (-orientation_.getZAxis()).normalized();
    float s = vne::math::clamp(f.dot(wu), -0.999f, 0.999f);
    float pitch_rad = std::asin(s);
    const float lim = vne::math::degToRad(kPitchMaxDeg);
    if (pitch_rad <= lim && pitch_rad >= -lim) {
        return;
    }
    const float target = vne::math::clamp(pitch_rad, -lim, lim);
    const float excess = pitch_rad - target;
    const vne::math::Vec3f right_axis = orientation_.getXAxis();
    const float rl = right_axis.length();
    if (rl < kEpsilon) {
        return;
    }
    orientation_ =
        (vne::math::Quatf::fromAxisAngle(right_axis / rl, -excess) * orientation_).normalized();
}

float FreeLookManipulator::getYawDegrees() const noexcept {
    float y = 0.0f;
    float p = 0.0f;
    yawPitchFromOrientation(y, p);
    return y;
}

float FreeLookManipulator::getPitchDegrees() const noexcept {
    float y = 0.0f;
    float p = 0.0f;
    yawPitchFromOrientation(y, p);
    return p;
}

void FreeLookManipulator::setOrientation(const vne::math::Quatf& q) noexcept {
    orientation_ = q.normalized();
    orientation_dirty_ = false;
    if (camera_) {
        applyOrientationToCamera();
    }
}

void FreeLookManipulator::applyDolly(float factor, float mx, float my) noexcept {
    if (orthoCamera()) {
        CameraManipulatorBase::applyDolly(factor, mx, my);
        return;
    }
    if (!camera_) {
        return;
    }
    ensureAnglesSynced();
    const vne::math::Vec3f f = camera_->getForwardDir();
    const float current_dist = (camera_->getTarget() - camera_->getPosition()).length();
    const float effective_factor = std::pow(factor, zoom_speed_);
    const float step = (1.0f - effective_factor) * std::max(current_dist, kEpsilon);
    camera_->setPosition(camera_->getPosition() + f * step);
    camera_->updateMatrices();
}

void FreeLookManipulator::setWorldUp(const vne::math::Vec3f& up) noexcept {
    if (up.length() > kEpsilon) {
        world_up_ = up.normalized();
    } else {
        VNE_LOG_WARN << "FreeLookManipulator: setWorldUp called with zero-length vector, ignoring";
    }
}

void FreeLookManipulator::setYawPitchDegrees(float yaw_deg, float pitch_deg) noexcept {
    if (!camera_) {
        return;
    }
    const float pitch_use =
        (mode_ == FreeLookMode::eFps) ? vne::math::clamp(pitch_deg, kPitchMinDeg, kPitchMaxDeg) : pitch_deg;

    const vne::math::Vec3f up_b = upVector();
    vne::math::Vec3f ref_fwd, ref_right;
    buildReferenceFrame(up_b, ref_fwd, ref_right);

    const float yaw_rad = vne::math::degToRad(yaw_deg);
    const float pitch_rad = vne::math::degToRad(pitch_use);
    const float cp = vne::math::cos(pitch_rad);
    vne::math::Vec3f f =
        (ref_fwd * vne::math::cos(yaw_rad) + ref_right * vne::math::sin(yaw_rad)) * cp + up_b * vne::math::sin(pitch_rad);
    const float fl = f.length();
    f = (fl < kEpsilon) ? ref_fwd : (f / fl);

    const vne::math::Vec3f pos = camera_->getPosition();
    vne::math::Vec3f up_apply;
    if (mode_ == FreeLookMode::eFps) {
        up_apply = world_up_;
    } else {
        const vne::math::Vec3f up_hint = upVector();
        vne::math::Vec3f r = f.cross(up_hint);
        float r_len = r.length();
        if (r_len < kEpsilon) {
            r = f.cross(world_up_);
            r_len = r.length();
        }
        if (r_len < kEpsilon) {
            buildReferenceFrame(world_up_, ref_fwd, ref_right);
            r = ref_right;
        }
        up_apply = r.cross(f).normalized();
    }

    camera_->lookAt(pos, pos + f, up_apply);
    camera_->updateMatrices();
    syncOrientationFromCamera();
    orientation_dirty_ = false;
}

float FreeLookManipulator::getWorldUnitsPerPixel() const noexcept {
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return kPerspWorldUnitsScale * vne::math::tan(fov_y_rad * kHalf) / viewport().height;
    }
    return 1.0f;
}

void FreeLookManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    ensureAnglesSynced();
    const vne::math::Vec3f center = (min_world + max_world) * kHalf;
    float radius = (max_world - min_world).length() * kHalf;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }
    const vne::math::Vec3f f = front();
    vne::math::Vec3f eye;
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float dist = (radius / vne::math::tan(fov_y_rad * kHalf)) * kFitToAabbMargin;
        eye = center - f * dist;
    } else {
        eye = center - f * (radius * kFitToAabbDistFactor);
    }
    const vne::math::Vec3f up = (mode_ == FreeLookMode::eFps) ? world_up_ : upVector();
    camera_->lookAt(eye, center, up);
    camera_->updateMatrices();
    syncOrientationFromCamera();
    orientation_dirty_ = false;
}

void FreeLookManipulator::resetState() noexcept {
    input_state_ = FreeLookInputState{};
    syncOrientationFromCamera();
    orientation_dirty_ = false;
}

void FreeLookManipulator::onUpdate(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    ensureAnglesSynced();
    const auto dt = static_cast<float>(delta_time);
    if (dt <= 0.0f) {
        return;
    }
    vne::math::Vec3f forward_axis;
    vne::math::Vec3f right_axis;
    vne::math::Vec3f vertical_axis;
    const vne::math::Vec3f world_up_n = normalizedWorldUp(world_up_);
    if (mode_ == FreeLookMode::eFly) {
        vne::math::Vec3f u = upVector();
        const float ul = u.length();
        vertical_axis = (ul >= kEpsilon) ? (u / ul) : world_up_n;
    }
    if (auto persp = perspCamera()) {
        forward_axis = persp->getForward();
        right_axis = persp->getRight();
        if (mode_ == FreeLookMode::eFps) {
            vne::math::Vec3f u = persp->getUpDir();
            const float ul = u.length();
            if (ul >= kEpsilon) {
                vertical_axis = u / ul;
            } else {
                const vne::math::Vec3f from_basis = right_axis.cross(forward_axis);
                const float bl = from_basis.length();
                vertical_axis = (bl >= kEpsilon) ? (from_basis / bl) : world_up_n;
            }
        }
    } else if (auto ortho = orthoCamera()) {
        if (mode_ == FreeLookMode::eFps) {
            vertical_axis = world_up_n;
        }
        const vne::math::Vec3f view_raw = ortho->getTarget() - ortho->getPosition();
        const float view_len = view_raw.length();
        const vne::math::Vec3f view_dir = (view_len >= kEpsilon) ? (view_raw / view_len) : front();
        forward_axis = orthoPanUp(*ortho, vertical_axis);
        const vne::math::Vec3f r_try = view_dir.cross(forward_axis);
        const float r_len = r_try.length();
        right_axis = (r_len >= kEpsilon) ? (r_try / r_len) : right(view_dir);
    } else {
        if (mode_ == FreeLookMode::eFps) {
            vertical_axis = world_up_n;
        }
        const vne::math::Vec3f f_raw = camera_->getTarget() - camera_->getPosition();
        const float f_len = f_raw.length();
        const vne::math::Vec3f view_dir = (f_len >= kEpsilon) ? (f_raw / f_len) : front();
        forward_axis = orthoPanUp(view_dir, vertical_axis);
        const vne::math::Vec3f r_try = view_dir.cross(forward_axis);
        const float r_len = r_try.length();
        right_axis = (r_len >= kEpsilon) ? (r_try / r_len) : right(view_dir);
    }
    vne::math::Vec3f move(0.0f, 0.0f, 0.0f);
    if (input_state_.move_forward) {
        move += forward_axis;
    }
    if (input_state_.move_backward) {
        move -= forward_axis;
    }
    if (input_state_.move_right) {
        move += right_axis;
    }
    if (input_state_.move_left) {
        move -= right_axis;
    }
    if (input_state_.move_up) {
        move += vertical_axis;
    }
    if (input_state_.move_down) {
        move -= vertical_axis;
    }
    const float move_len = move.length();
    if (move_len <= kEpsilon) {
        return;
    }
    float speed = move_speed_;
    if (input_state_.sprint) {
        speed *= sprint_mult_;
    } else if (input_state_.slow) {
        speed *= slow_mult_;
    }
    const float scene_s = camera_->getSceneScale();
    const float scene_comp = (scene_s > kEpsilon) ? (1.0f / scene_s) : 1.0f;
    move = (move / move_len) * (speed * dt * scene_comp);
    camera_->setPosition(camera_->getPosition() + move);
    camera_->updateMatrices();
}

bool FreeLookManipulator::onAction(CameraActionType action,
                                   const CameraCommandPayload& payload,
                                   double /*delta_time*/) noexcept {
    if (!enabled_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eBeginLook:
            ensureAnglesSynced();
            input_state_.looking = true;
            return true;

        case CameraActionType::eEndLook:
            input_state_.looking = false;
            return true;

        case CameraActionType::eLookDelta:
            if (camera_ && input_state_.looking) {
                ensureAnglesSynced();
                // Match legacy yaw += delta_x * sens: negative delta_x turns view left (forward.x decreases from +Z eye).
                const float yaw_rad = -vne::math::degToRad(payload.delta_x_px * mouse_sensitivity_);
                const float pitch_rad = -vne::math::degToRad(payload.delta_y_px * mouse_sensitivity_);
                const vne::math::Vec3f wu_n = normalizedWorldUp(world_up_);
                if (mode_ == FreeLookMode::eFps) {
                    const vne::math::Quatf dq_yaw = vne::math::Quatf::fromAxisAngle(wu_n, yaw_rad);
                    orientation_ = (dq_yaw * orientation_).normalized();
                    const vne::math::Vec3f right_ax = orientation_.getXAxis();
                    const float rl = right_ax.length();
                    if (rl >= kEpsilon) {
                        const vne::math::Quatf dq_pitch =
                            vne::math::Quatf::fromAxisAngle(right_ax / rl, pitch_rad);
                        orientation_ = (dq_pitch * orientation_).normalized();
                    }
                    clampFpsPitch();
                } else {
                    const vne::math::Vec3f local_up = orientation_.getYAxis();
                    float ul = local_up.length();
                    const vne::math::Vec3f up_n = (ul >= kEpsilon) ? (local_up / ul) : wu_n;
                    const vne::math::Quatf dq_yaw = vne::math::Quatf::fromAxisAngle(up_n, yaw_rad);
                    orientation_ = (dq_yaw * orientation_).normalized();
                    vne::math::Vec3f right_ax = orientation_.getXAxis();
                    ul = right_ax.length();
                    if (ul >= kEpsilon) {
                        const vne::math::Quatf dq_pitch =
                            vne::math::Quatf::fromAxisAngle(right_ax / ul, pitch_rad);
                        orientation_ = (dq_pitch * orientation_).normalized();
                    }
                }
                applyOrientationToCamera();
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
