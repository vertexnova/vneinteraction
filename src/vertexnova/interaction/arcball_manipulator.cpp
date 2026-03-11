/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kMaxOrbitDistance = 1e6f;
constexpr float kInertiaRotSpeedMax = 10.0f;
constexpr float kInertiaRotAngleThreshold = 1e-6f;
constexpr float kInertiaRotSpeedThreshold = 1e-4f;
constexpr float kInertiaPanSpeedThreshold = 1e-4f;
constexpr float kTouchPanViewportCenterFactor = 0.5f;
constexpr double kMinDeltaTimeForInertia = 0.001;  // 1ms: ignore tiny-dt inertia samples
constexpr uint32_t kNormalizeEveryNFrames = 60u;   // renormalize orientation_ periodically

float safeSqrt(float x) noexcept {
    return std::sqrt(std::max(0.0f, x));
}
float scrollToZoomFactor(float scroll_y, float zoom_speed) noexcept {
    return (scroll_y > 0.0f) ? (1.0f / zoom_speed) : zoom_speed;
}
}  // namespace

ArcballManipulator::ArcballManipulator() noexcept {
    world_up_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    coi_world_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    inertia_rot_axis_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    rotation_speed_ = 1.0f;
}

void ArcballManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncFromCamera();
}

void ArcballManipulator::setViewportSize(float width_px, float height_px) noexcept {
    CameraManipulatorBase::setViewportSize(width_px, height_px);
    if (auto persp = perspCamera()) {
        persp->setViewport(viewport_width_, viewport_height_);
    }
}

void ArcballManipulator::setWorldUp(const vne::math::Vec3f& world_up) noexcept {
    if (world_up.length() < kEpsilon) {
        return;
    }
    world_up_ = world_up.normalized();
}

void ArcballManipulator::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = vne::math::clamp(distance, kMinOrbitDistance, kMaxOrbitDistance);
    applyToCamera();
}

void ArcballManipulator::setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept {
    if (!camera_) {
        coi_world_ = coi;
        return;
    }
    if (space == CenterOfInterestSpace::eWorldSpace) {
        coi_world_ = coi;
    } else {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f r = computeRight(front);
        const vne::math::Vec3f u = computeUp(front, r);
        coi_world_ = camera_->getPosition() + r * coi.x() + u * coi.y() + front * coi.z();
    }
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
}

void ArcballManipulator::syncFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    coi_world_ = camera_->getTarget();
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);

    // Build orientation_ from the full camera basis (front + up), preserving any existing roll.
    // orientation_ maps the rest frame (+Z = eye-from-COI, +Y = up) to the current camera frame.
    vne::math::Vec3f back = (camera_->getPosition() - coi_world_);
    const float back_len = back.length();
    back = (back_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, 1.0f) : (back / back_len);

    vne::math::Vec3f up = camera_->getUp();
    const float up_len = up.length();
    up = (up_len < kEpsilon) ? world_up_ : (up / up_len);

    // Re-orthogonalize: right = up × back (then recompute up = back × right)
    vne::math::Vec3f right = up.cross(back);
    const float right_len = right.length();
    if (right_len < kEpsilon) {
        // Degenerate: up parallel to back, fall back to world_up_-derived frame
        right = world_up_.cross(back);
        const float r2 = right.length();
        right = (r2 < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (right / r2);
    } else {
        right /= right_len;
    }
    up = back.cross(right);  // guaranteed orthogonal to back and right

    // Build rotation matrix from basis columns: col0=right(+X), col1=up(+Y), col2=back(+Z)
    // glm::quat_cast expects a matrix whose columns are the basis vectors of the target frame.
    const vne::math::Mat4f rot(vne::math::Vec4f(right.x(), right.y(), right.z(), 0.0f),
                               vne::math::Vec4f(up.x(), up.y(), up.z(), 0.0f),
                               vne::math::Vec4f(back.x(), back.y(), back.z(), 0.0f),
                               vne::math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    orientation_ = vne::math::Quatf(rot).normalized();
    normalize_counter_ = 0;
}

void ArcballManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    // Derive eye position and up from persistent orientation_ — no float drift.
    const vne::math::Vec3f eye_dir = orientation_.rotate(vne::math::Vec3f(0.0f, 0.0f, 1.0f));
    const vne::math::Vec3f up = orientation_.rotate(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->setPosition(coi_world_ + eye_dir * orbit_distance_);
    camera_->setTarget(coi_world_);
    camera_->setUp(up);
    camera_->updateMatrices();
}

vne::math::Vec3f ArcballManipulator::computeFront() const noexcept {
    if (!camera_) {
        return vne::math::Vec3f(0.0f, 0.0f, -1.0f);
    }
    vne::math::Vec3f dir = camera_->getTarget() - camera_->getPosition();
    const float len = dir.length();
    return (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (dir / len);
}

vne::math::Vec3f ArcballManipulator::projectToArcball(float x_px, float y_px) const noexcept {
    // Pure screen-space ARC mode (Song Ho Ahn's trackball algorithm).
    // Returns a unit vector in camera-space: X=right, Y=up, Z=toward viewer.
    // No camera basis vectors involved — the same pixel always maps to the same vector
    // regardless of current camera orientation, eliminating drift/jitter.
    const float half_size = 0.5f * std::min(viewport_width_, viewport_height_);
    const float cx = viewport_width_ * 0.5f;
    const float cy = viewport_height_ * 0.5f;
    const float sx = (x_px - cx) / half_size;
    const float sy = (cy - y_px) / half_size;  // flip Y: screen-down is world-down
    const float r2 = sx * sx + sy * sy;
    float vx, vy, vz;
    if (r2 <= 1.0f) {
        // Inside sphere: use arc-length parameterization
        const float arc = safeSqrt(r2);
        const float a = arc * (vne::math::kPi * 0.5f);  // arc length → angle on sphere
        const float sin_a = vne::math::sin(a);
        const float s = (arc > kEpsilon) ? (sin_a / arc) : 1.0f;
        vx = sx * s;
        vy = sy * s;
        vz = vne::math::cos(a);
    } else {
        // Outside sphere: clamp to equator
        const float inv_len = 1.0f / safeSqrt(r2);
        vx = sx * inv_len;
        vy = sy * inv_len;
        vz = 0.0f;
    }
    return vne::math::Vec3f(vx, vy, vz).normalized();
}

void ArcballManipulator::applyCommand(CameraActionType action,
                                      const CameraCommandPayload& payload,
                                      double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    switch (action) {
        case CameraActionType::eBeginRotate:
            beginRotate(payload.x_px, payload.y_px);
            break;
        case CameraActionType::eRotateDelta:
            if (interaction_.rotating) {
                dragRotate(payload.x_px, payload.y_px, delta_time);
            }
            break;
        case CameraActionType::eEndRotate:
            endRotate(delta_time);
            break;
        case CameraActionType::eBeginPan:
            beginPan(payload.x_px, payload.y_px);
            break;
        case CameraActionType::ePanDelta:
            if (interaction_.panning) {
                dragPan(payload.x_px, payload.y_px, payload.delta_x_px, payload.delta_y_px, delta_time);
                interaction_.last_x_px = payload.x_px;
                interaction_.last_y_px = payload.y_px;
            }
            break;
        case CameraActionType::eEndPan:
            endPan(delta_time);
            break;
        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f) {
                zoom(payload.zoom_factor, payload.x_px, payload.y_px);
            }
            break;
        case CameraActionType::eResetView:
            resetState();
            break;
        default:
            OrbitStyleBase::applyCommand(action, payload, delta_time);
            break;
    }
}

void ArcballManipulator::beginRotate(float x_px, float y_px) noexcept {
    if (!camera_) {
        return;
    }
    interaction_.rotating = true;
    // Store screen-space start position — stable across frames (no world transform)
    arcball_start_x_ = x_px;
    arcball_start_y_ = y_px;
    inertia_rot_speed_ = 0.0f;
}

void ArcballManipulator::dragRotate(float x_px, float y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    // Project both positions in stable screen-space (no camera vectors → no drift)
    const vne::math::Vec3f prev_cam = projectToArcball(arcball_start_x_, arcball_start_y_);
    const vne::math::Vec3f curr_cam = projectToArcball(x_px, y_px);

    // Transform to world-space ONCE using this frame's camera orientation
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    // Swap prev/curr so scene follows cursor (drag-the-world convention)
    const vne::math::Vec3f prev_world = (r * curr_cam.x() + u * curr_cam.y() + front * curr_cam.z()).normalized();
    const vne::math::Vec3f curr_world = (r * prev_cam.x() + u * prev_cam.y() + front * prev_cam.z()).normalized();

    // Update screen-space start for next frame
    arcball_start_x_ = x_px;
    arcball_start_y_ = y_px;

    const float dot = vne::math::clamp(prev_world.dot(curr_world), -1.0f, 1.0f);
    if (dot >= 1.0f - kEpsilon) {
        return;
    }
    vne::math::Vec3f axis = prev_world.cross(curr_world);
    const float axis_len = axis.length();
    if (axis_len < kEpsilon) {
        return;
    }
    axis /= axis_len;
    const float angle = vne::math::acos(dot) * rotation_speed_;

    // Left-multiply delta quaternion onto persistent orientation_ (world-space rotation)
    const vne::math::Quatf delta_q = vne::math::Quatf::fromAxisAngle(axis, angle);
    orientation_ = delta_q * orientation_;

    // Periodic renormalization to prevent floating-point drift
    if (++normalize_counter_ >= kNormalizeEveryNFrames) {
        orientation_ = orientation_.normalized();
        normalize_counter_ = 0;
    }

    applyToCamera();

    if (delta_time >= kMinDeltaTimeForInertia && std::abs(angle) > kInertiaRotAngleThreshold) {
        inertia_rot_axis_ = axis;
        inertia_rot_speed_ =
            vne::math::clamp(angle / static_cast<float>(delta_time), -kInertiaRotSpeedMax, kInertiaRotSpeedMax);
    }
}

void ArcballManipulator::endRotate(double) noexcept {
    interaction_.rotating = false;
}

void ArcballManipulator::applyInertia(double delta_time) noexcept {
    if (!camera_ || delta_time <= 0.0) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    bool changed = false;

    if (std::abs(inertia_rot_speed_) > kInertiaRotSpeedThreshold) {
        // Apply rotation inertia through persistent orientation_ (consistent with dragRotate)
        const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(inertia_rot_axis_, inertia_rot_speed_ * dt);
        orientation_ = (q * orientation_).normalized();
        inertia_rot_speed_ *= std::exp(-rot_damping_ * dt);
        changed = true;
    }

    if (inertia_pan_velocity_.length() > kInertiaPanSpeedThreshold) {
        const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
        inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
        if (pivot_mode_ == RotationPivotMode::eFixedWorld) {
            // Fixed pivot: translate eye+target together, leave coi_world_ pinned
            camera_->setPosition(camera_->getPosition() + delta);
            camera_->setTarget(camera_->getTarget() + delta);
            camera_->updateMatrices();
            orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
            changed = false;  // applyToCamera() would overwrite eye position from orientation_
        } else {
            coi_world_ += delta;
            changed = true;
        }
    }

    if (changed) {
        applyToCamera();
    }
}

void ArcballManipulator::resetState() noexcept {
    OrbitStyleBase::resetState();
    inertia_rot_speed_ = 0.0f;
    inertia_rot_axis_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    arcball_start_x_ = 0.0f;
    arcball_start_y_ = 0.0f;
    normalize_counter_ = 0;
    // Re-derive orientation_ from current camera state
    syncFromCamera();
}

void ArcballManipulator::onPivotChanged() noexcept {
    // Called when coi_world_ changes (pivot mode switch, pan end in eViewCenter, fitToAABB).
    // Re-derive orientation_ so dragRotate/applyInertia stay consistent with new COI.
    syncFromCamera();
}

void ArcballManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    OrbitStyleBase::fitToAABB(min_world, max_world);
}

float ArcballManipulator::getWorldUnitsPerPixel() const noexcept {
    return OrbitStyleBase::getWorldUnitsPerPixel();
}

void ArcballManipulator::handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (interaction_.rotating) {
        // Arcball needs absolute screen positions for projectToArcball; base passes deltas
        dragRotate(x, y, delta_time);
    } else if (interaction_.panning) {
        dragPan(x, y, delta_x, delta_y, delta_time);
    }
    interaction_.last_x_px = x;
    interaction_.last_y_px = y;
}

void ArcballManipulator::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    zoom(scrollToZoomFactor(scroll_y, zoom_speed_), mouse_x, mouse_y);
}

void ArcballManipulator::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!interaction_.rotating) {
        const float center_x = viewport_width_ * kTouchPanViewportCenterFactor;
        const float center_y = viewport_height_ * kTouchPanViewportCenterFactor;
        beginRotate(center_x, center_y);
        interaction_.last_x_px = center_x;
        interaction_.last_y_px = center_y;
    }
    const float x = interaction_.last_x_px + pan.delta_x_px;
    const float y = interaction_.last_y_px + pan.delta_y_px;
    dragRotate(x, y, delta_time);
    interaction_.last_x_px = x;
    interaction_.last_y_px = y;
}

}  // namespace vne::interaction
