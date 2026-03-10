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
constexpr float kArcballSphereRadiusSq = 1.0f;
constexpr float kInertiaRotSpeedMax = 10.0f;
constexpr float kInertiaRotAngleThreshold = 1e-6f;
constexpr float kInertiaRotSpeedThreshold = 1e-4f;
constexpr float kInertiaPanSpeedThreshold = 1e-4f;
constexpr float kTouchPanViewportCenterFactor = 0.5f;

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
}

void ArcballManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    vne::math::Vec3f dir = camera_->getPosition() - coi_world_;
    const float len = dir.length();
    dir = (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, 1.0f) : (dir / len);
    camera_->setPosition(coi_world_ + dir * orbit_distance_);
    camera_->setTarget(coi_world_);
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
    const float half_size = 0.5f * std::min(viewport_width_, viewport_height_);
    const float cx = viewport_width_ * 0.5f;
    const float cy = viewport_height_ * 0.5f;
    float ax = (x_px - cx) / half_size;
    float ay = (cy - y_px) / half_size;
    const float r2 = ax * ax + ay * ay;
    float az = 0.0f;
    if (r2 <= kArcballSphereRadiusSq) {
        az = safeSqrt(kArcballSphereRadiusSq - r2);
    } else {
        const float inv_len = 1.0f / std::sqrt(r2);
        ax *= inv_len;
        ay *= inv_len;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    return (r * ax + u * ay + front * az).normalized();
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
    arcball_start_world_ = projectToArcball(x_px, y_px);
    inertia_rot_speed_ = 0.0f;
}

void ArcballManipulator::dragRotate(float x_px, float y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f prev = arcball_start_world_;
    const vne::math::Vec3f curr = projectToArcball(x_px, y_px);
    arcball_start_world_ = curr;
    const float dot = vne::math::clamp(prev.dot(curr), -1.0f, 1.0f);
    if (dot >= 1.0f - kEpsilon) {
        return;
    }
    vne::math::Vec3f axis = prev.cross(curr);
    const float axis_len = axis.length();
    if (axis_len < kEpsilon) {
        return;
    }
    axis /= axis_len;
    const float angle = vne::math::acos(dot) * rotation_speed_;
    const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(axis, angle);
    const vne::math::Vec3f eye_offset = camera_->getPosition() - coi_world_;
    const vne::math::Vec3f up = camera_->getUp();
    camera_->setPosition(coi_world_ + q.rotate(eye_offset));
    camera_->setUp(q.rotate(up));
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    if (delta_time > 0.0 && std::abs(angle) > kInertiaRotAngleThreshold) {
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
        const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(inertia_rot_axis_, inertia_rot_speed_ * dt);
        const vne::math::Vec3f eye_offset = camera_->getPosition() - coi_world_;
        const vne::math::Vec3f up = camera_->getUp();
        camera_->setPosition(coi_world_ + q.rotate(eye_offset));
        camera_->setUp(q.rotate(up));
        camera_->setTarget(coi_world_);
        inertia_rot_speed_ *= std::exp(-rot_damping_ * dt);
        changed = true;
    }
    if (inertia_pan_velocity_.length() > kInertiaPanSpeedThreshold) {
        const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
        coi_world_ += delta;
        camera_->setPosition(camera_->getPosition() + delta);
        camera_->setTarget(coi_world_);
        inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
        changed = true;
    }
    if (changed) {
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        camera_->updateMatrices();
    }
}

void ArcballManipulator::resetState() noexcept {
    OrbitStyleBase::resetState();
    inertia_rot_speed_ = 0.0f;
    inertia_rot_axis_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    interaction_.last_x_px = 0.0f;
    interaction_.last_y_px = 0.0f;
}

void ArcballManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    OrbitStyleBase::fitToAABB(min_world, max_world);
}

float ArcballManipulator::getWorldUnitsPerPixel() const noexcept {
    return OrbitStyleBase::getWorldUnitsPerPixel();
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
        beginRotate(viewport_width_ * kTouchPanViewportCenterFactor, viewport_height_ * kTouchPanViewportCenterFactor);
    }
    const float x = interaction_.last_x_px + pan.delta_x_px;
    const float y = interaction_.last_y_px + pan.delta_y_px;
    dragRotate(x, y, delta_time);
    interaction_.last_x_px = x;
    interaction_.last_y_px = y;
}

}  // namespace vne::interaction
