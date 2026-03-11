/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kDefaultOrbitDistance = 5.0f;
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;
constexpr float kInertiaRotThreshold = 1e-3f;
constexpr double kMinDeltaTimeForInertia = 0.001;  // 1ms: ignore tiny-dt inertia samples

/// Build a reference forward and right vector from an arbitrary world-up.
/// For Y-up: forward = -Z, right = +X.  For Z-up: forward = +Y, right = +X.
void buildReferenceFrame(const vne::math::Vec3f& world_up,
                         vne::math::Vec3f& ref_fwd,
                         vne::math::Vec3f& ref_right) noexcept {
    // Choose a candidate forward that is not parallel to world_up
    vne::math::Vec3f candidate =
        (std::abs(world_up.y()) > 0.9f) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : vne::math::Vec3f(0.0f, -1.0f, 0.0f);
    // Gram-Schmidt: make candidate perpendicular to world_up
    ref_fwd = (candidate - world_up * candidate.dot(world_up));
    const float fwd_len = ref_fwd.length();
    ref_fwd = (fwd_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = world_up.cross(ref_fwd);
    const float right_len = ref_right.length();
    if (right_len > kEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}
}  // namespace

OrbitManipulator::OrbitManipulator() noexcept {
    world_up_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    coi_world_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitManipulator::setWorldUp(const vne::math::Vec3f& world_up) noexcept {
    if (world_up.length() > kEpsilon) {
        world_up_ = world_up.normalized();
    }
}

void OrbitManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncFromCamera();
}

vne::math::Vec3f OrbitManipulator::computeFront() const noexcept {
    vne::math::Vec3f ref_fwd, ref_right;
    buildReferenceFrame(world_up_, ref_fwd, ref_right);

    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = vne::math::cos(pitch_rad);
    // Rotate ref_fwd by yaw around world_up_, then tilt by pitch toward world_up_
    vne::math::Vec3f front = (ref_fwd * vne::math::cos(yaw_rad) + ref_right * vne::math::sin(yaw_rad)) * cp
                             + world_up_ * vne::math::sin(pitch_rad);
    const float len = front.length();
    return (len < kEpsilon) ? ref_fwd : (front / len);
}

void OrbitManipulator::syncFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f eye = camera_->getPosition();
    coi_world_ = camera_->getTarget();
    vne::math::Vec3f front = coi_world_ - eye;
    orbit_distance_ = front.length();
    if (orbit_distance_ < kEpsilon) {
        orbit_distance_ = kDefaultOrbitDistance;
        yaw_deg_ = 0.0f;
        pitch_deg_ = 0.0f;
        return;
    }
    front /= orbit_distance_;

    // Extract pitch: component along world_up_
    const float up_comp = vne::math::clamp(front.dot(world_up_), -1.0f, 1.0f);
    pitch_deg_ = vne::math::radToDeg(vne::math::asin(up_comp));

    // Extract yaw: project front onto the horizontal plane, measure against reference frame
    const vne::math::Vec3f horiz = front - world_up_ * up_comp;
    const float horiz_len = horiz.length();
    if (horiz_len < kEpsilon) {
        yaw_deg_ = 0.0f;
        return;
    }
    const vne::math::Vec3f horiz_n = horiz / horiz_len;
    vne::math::Vec3f ref_fwd, ref_right;
    buildReferenceFrame(world_up_, ref_fwd, ref_right);
    yaw_deg_ = vne::math::radToDeg(vne::math::atan2(horiz_n.dot(ref_right), horiz_n.dot(ref_fwd)));
}

void OrbitManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f right = computeRight(front);
    const vne::math::Vec3f up = computeUp(front, right);
    const vne::math::Vec3f eye = coi_world_ - front * orbit_distance_;
    camera_->setPosition(eye);
    camera_->setTarget(coi_world_);
    camera_->setUp(up);
    camera_->updateMatrices();
}

void OrbitManipulator::setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept {
    if (space == CenterOfInterestSpace::eWorldSpace || !camera_) {
        coi_world_ = coi;
    } else {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f right = computeRight(front);
        const vne::math::Vec3f up = computeUp(front, right);
        const vne::math::Vec3f eye = camera_->getPosition();
        coi_world_ = eye + right * coi.x() + up * coi.y() + front * coi.z();
    }
    applyToCamera();
}

void OrbitManipulator::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = std::max(distance, kMinOrbitDistance);
    applyToCamera();
}

void OrbitManipulator::setViewDirection(ViewDirection dir) noexcept {
    switch (dir) {
        case ViewDirection::eFront:
            yaw_deg_ = 0.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eBack:
            yaw_deg_ = 180.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eLeft:
            yaw_deg_ = -90.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eRight:
            yaw_deg_ = 90.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eTop:
            yaw_deg_ = 0.0f;
            pitch_deg_ = 89.0f;
            break;
        case ViewDirection::eBottom:
            yaw_deg_ = 0.0f;
            pitch_deg_ = -89.0f;
            break;
        case ViewDirection::eIso:
            yaw_deg_ = 45.0f;
            pitch_deg_ = 35.2643897f;
            break;
    }
    applyToCamera();
}

void OrbitManipulator::beginRotate(float x_px, float y_px) noexcept {
    interaction_.rotating = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
}

void OrbitManipulator::dragRotate(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    yaw_deg_ -= delta_x_px * rotation_speed_;
    pitch_deg_ += delta_y_px * rotation_speed_;
    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
    applyToCamera();
    if (delta_time >= kMinDeltaTimeForInertia) {
        const float inv_dt = 1.0f / static_cast<float>(delta_time);
        inertia_rot_speed_x_ = -delta_x_px * rotation_speed_ * inv_dt;
        inertia_rot_speed_y_ = delta_y_px * rotation_speed_ * inv_dt;
    }
}

void OrbitManipulator::endRotate(double) noexcept {
    interaction_.rotating = false;
}

void OrbitManipulator::applyInertia(double delta_time) noexcept {
    if (delta_time <= 0.0 || !camera_) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    if (std::abs(inertia_rot_speed_x_) > kInertiaRotThreshold
        || std::abs(inertia_rot_speed_y_) > kInertiaRotThreshold) {
        yaw_deg_ += inertia_rot_speed_x_ * dt;
        pitch_deg_ += inertia_rot_speed_y_ * dt;
        pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
        const float rot_decay = std::exp(-rot_damping_ * dt);
        inertia_rot_speed_x_ *= rot_decay;
        inertia_rot_speed_y_ *= rot_decay;
        applyToCamera();
    }
    doPanInertia(delta_time);
}

void OrbitManipulator::resetState() noexcept {
    OrbitStyleBase::resetState();
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
}

void OrbitManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    OrbitStyleBase::fitToAABB(min_world, max_world);
}

float OrbitManipulator::getWorldUnitsPerPixel() const noexcept {
    return OrbitStyleBase::getWorldUnitsPerPixel();
}

}  // namespace vne::interaction
