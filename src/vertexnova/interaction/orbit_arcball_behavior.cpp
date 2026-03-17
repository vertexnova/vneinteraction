/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbit_arcball_behavior.h"
#include "vertexnova/interaction/behavior_math.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/math/easing.h>

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Anonymous constants (mirrors orbit_style_base.cpp / arcball_manipulator.cpp)
// ---------------------------------------------------------------------------
namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.orbit_arcball");
constexpr float kEpsilon = 1e-6f;
constexpr float kFrontZNearVertical = 0.999f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kMaxOrbitDistance = 1e6f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kInertiaPanThreshold = 1e-4f;
constexpr float kPanVelocityBlend = 0.35f;
constexpr float kFitAnimationSpeed = 10.0f;
constexpr float kFitConvergeThreshold = 1e-3f;
constexpr double kMinDeltaTimeForInertia = 0.001;
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;
constexpr float kInertiaRotThreshold = 1e-3f;
constexpr float kDefaultOrbitDistance = 5.0f;
constexpr float kInertiaRotSpeedMax = 10.0f;
constexpr float kInertiaRotAngleThreshold = 1e-6f;
constexpr float kInertiaRotSpeedThreshold = 1e-4f;
constexpr float kInertiaPanSpeedThreshold = 1e-4f;
constexpr uint32_t kNormalizeEveryNFrames = 60u;

float safeSqrt(float x) noexcept {
    return std::sqrt(std::max(0.0f, x));
}
}  // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

OrbitArcballBehavior::OrbitArcballBehavior() noexcept
    : orientation_(0.0f, 0.0f, 0.0f, 1.0f) {
    world_up_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    coi_world_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    inertia_rot_axis_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// Camera helpers
// ---------------------------------------------------------------------------

bool OrbitArcballBehavior::isPerspective() const noexcept {
    return static_cast<bool>(perspCamera());
}

bool OrbitArcballBehavior::isOrthographic() const noexcept {
    return static_cast<bool>(orthoCamera());
}

// ---------------------------------------------------------------------------
// ICameraBehavior: setCamera / onResize
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraBehaviorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "OrbitArcballBehavior: camera detached (null camera)";
    }
    syncFromCamera();
}

void OrbitArcballBehavior::onResize(float width_px, float height_px) noexcept {
    CameraBehaviorBase::onResize(width_px, height_px);
    if (auto persp = perspCamera()) {
        persp->resize(viewport().width, viewport().height);
    }
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

vne::math::Vec3f OrbitArcballBehavior::computeRight(const vne::math::Vec3f& front) const noexcept {
    // front × up = right  (standard RH rule; replaces the old up × front = left)
    vne::math::Vec3f r = front.cross(world_up_);
    float len = r.length();
    if (len < kEpsilon) {
        // front is collinear with world_up — use a fallback up axis
        const vne::math::Vec3f fallback_up = (std::abs(front.z()) < kFrontZNearVertical)
                                                 ? vne::math::Vec3f(0.0f, 0.0f, 1.0f)
                                                 : vne::math::Vec3f(0.0f, 1.0f, 0.0f);
        r = front.cross(fallback_up);
        len = r.length();
    }
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

vne::math::Vec3f OrbitArcballBehavior::computeUp(const vne::math::Vec3f& front,
                                                 const vne::math::Vec3f& right) const noexcept {
    // right × front = up  (standard RH rule; replaces the old front × right = -up)
    const vne::math::Vec3f up = right.cross(front);
    const float len = up.length();
    return (len < kEpsilon) ? world_up_ : (up / len);
}

// ---------------------------------------------------------------------------
// Euler-mode: computeFront
// (Quaternion-mode derives front from orientation_ in applyToCamera)
// ---------------------------------------------------------------------------

vne::math::Vec3f OrbitArcballBehavior::computeFront() const noexcept {
    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        return -orientation_.getZAxis();
    }
    // Euler
    vne::math::Vec3f ref_fwd, ref_right;
    buildReferenceFrame(world_up_, ref_fwd, ref_right);
    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = vne::math::cos(pitch_rad);
    vne::math::Vec3f front = (ref_fwd * vne::math::cos(yaw_rad) + ref_right * vne::math::sin(yaw_rad)) * cp
                             + world_up_ * vne::math::sin(pitch_rad);
    const float len = front.length();
    return (len < kEpsilon) ? ref_fwd : (front / len);
}

// ---------------------------------------------------------------------------
// syncFromCamera / applyToCamera
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::syncFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    coi_world_ = camera_->getTarget();
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);

    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        // Build orientation_ from the full camera basis — same as ArcballManipulator::syncFromCamera
        vne::math::Vec3f back = (camera_->getPosition() - coi_world_);
        const float back_len = back.length();
        back = (back_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, 1.0f) : (back / back_len);

        vne::math::Vec3f up = camera_->getUp();
        const float up_len = up.length();
        up = (up_len < kEpsilon) ? world_up_ : (up / up_len);

        vne::math::Vec3f right = up.cross(back);
        const float right_len = right.length();
        if (right_len < kEpsilon) {
            right = world_up_.cross(back);
            const float r2 = right.length();
            right = (r2 < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (right / r2);
        } else {
            right /= right_len;
        }
        up = back.cross(right);

        const vne::math::Mat4f rot(vne::math::Vec4f(right.x(), right.y(), right.z(), 0.0f),
                                   vne::math::Vec4f(up.x(), up.y(), up.z(), 0.0f),
                                   vne::math::Vec4f(back.x(), back.y(), back.z(), 0.0f),
                                   vne::math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
        orientation_ = vne::math::Quatf(rot).normalized();
        normalize_counter_ = 0;
    } else {
        // Euler: extract yaw/pitch from camera direction
        vne::math::Vec3f front = coi_world_ - camera_->getPosition();
        if (orbit_distance_ < kEpsilon) {
            orbit_distance_ = kDefaultOrbitDistance;
            yaw_deg_ = 0.0f;
            pitch_deg_ = 0.0f;
            return;
        }
        front /= orbit_distance_;

        const float up_comp = vne::math::clamp(front.dot(world_up_), -1.0f, 1.0f);
        pitch_deg_ = vne::math::radToDeg(vne::math::asin(up_comp));

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
}

void OrbitArcballBehavior::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        const vne::math::Vec3f back = orientation_.getZAxis();
        const vne::math::Vec3f up = orientation_.getYAxis();
        camera_->lookAt(coi_world_ + back * orbit_distance_, coi_world_, up);
    } else {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f up = computeUp(front, computeRight(front));
        camera_->lookAt(coi_world_ - front * orbit_distance_, coi_world_, up);
    }
    camera_->updateMatrices();
}

void OrbitArcballBehavior::onPivotChanged() noexcept {
    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        syncFromCamera();
    }
    // Euler: no extra sync needed; yaw/pitch are still valid after COI moves
}

// ---------------------------------------------------------------------------
// Rotation
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::beginRotate(float x_px, float y_px) noexcept {
    interaction_.rotating = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    arcball_start_x_ = x_px;
    arcball_start_y_ = y_px;
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
    inertia_rot_speed_ = 0.0f;
}

void OrbitArcballBehavior::dragRotateEuler(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    yaw_deg_ += delta_x_px * rotation_speed_;
    pitch_deg_ += delta_y_px * rotation_speed_;
    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
    applyToCamera();
    if (delta_time >= kMinDeltaTimeForInertia) {
        const float inv_dt = 1.0f / static_cast<float>(delta_time);
        inertia_rot_speed_x_ = delta_x_px * rotation_speed_ * inv_dt;
        inertia_rot_speed_y_ = delta_y_px * rotation_speed_ * inv_dt;
    }
}

vne::math::Vec3f OrbitArcballBehavior::projectToArcball(float x_px, float y_px) const noexcept {
    const float half_size = 0.5f * std::min(viewport().width, viewport().height);
    const float cx = viewport().width * 0.5f;
    const float cy = viewport().height * 0.5f;
    const float sx = (x_px - cx) / half_size;
    const float sy = (cy - y_px) / half_size;
    const float r2 = sx * sx + sy * sy;
    float vx, vy, vz;
    if (r2 <= 1.0f) {
        const float arc = safeSqrt(r2);
        const float a = arc * (vne::math::kPi * 0.5f);
        const float sin_a = vne::math::sin(a);
        const float s = (arc > kEpsilon) ? (sin_a / arc) : 1.0f;
        vx = sx * s;
        vy = sy * s;
        vz = vne::math::cos(a);
    } else {
        const float inv_len = 1.0f / safeSqrt(r2);
        vx = sx * inv_len;
        vy = sy * inv_len;
        vz = 0.0f;
    }
    return vne::math::Vec3f(vx, vy, vz).normalized();
}

void OrbitArcballBehavior::dragRotateArcball(float x_px, float y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    // Project both points in stable screen-space (no camera vectors)
    const vne::math::Vec3f prev_cam = projectToArcball(arcball_start_x_, arcball_start_y_);
    const vne::math::Vec3f curr_cam = projectToArcball(x_px, y_px);

    // Transform to world-space using quaternion-derived basis (pole-safe, no world_up cross)
    const vne::math::Vec3f r = orientation_.getXAxis();
    const vne::math::Vec3f u = orientation_.getYAxis();
    const vne::math::Vec3f front = -orientation_.getZAxis();

    // Map screen-space arcball vectors into world space.
    // With computeRight returning the true +right vector, the natural prev→curr
    // direction already implements drag-the-world — no swap needed.
    const vne::math::Vec3f prev_world = (r * prev_cam.x() + u * prev_cam.y() + front * prev_cam.z()).normalized();
    const vne::math::Vec3f curr_world = (r * curr_cam.x() + u * curr_cam.y() + front * curr_cam.z()).normalized();

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

    const vne::math::Quatf delta_q = vne::math::Quatf::fromAxisAngle(axis, angle);
    orientation_ = delta_q * orientation_;

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

void OrbitArcballBehavior::endRotate(double) noexcept {
    interaction_.rotating = false;
}

// ---------------------------------------------------------------------------
// Pan
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::beginPan(float x_px, float y_px) noexcept {
    interaction_.panning = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitArcballBehavior::dragPan(
    float /*x*/, float /*y*/, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    vne::math::Vec3f delta_world(0.0f, 0.0f, 0.0f);

    if (isPerspective()) {
        const float wpp = getWorldUnitsPerPixel();
        delta_world = r * (-delta_x_px * wpp * pan_speed_) + u * (-delta_y_px * wpp * pan_speed_);
    } else {
        auto ortho = orthoCamera();
        if (ortho) {
            const float wppx = ortho->getWidth() / viewport().width;
            const float wppy = ortho->getHeight() / viewport().height;
            delta_world = r * (-delta_x_px * wppx * pan_speed_) + u * (-delta_y_px * wppy * pan_speed_);
        }
    }

    if (pivot_mode_ == OrbitPivotMode::eFixed) {
        camera_->setPosition(camera_->getPosition() + delta_world);
        camera_->setTarget(camera_->getTarget() + delta_world);
        camera_->updateMatrices();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    } else {
        coi_world_ += delta_world;
        applyToCamera();
    }

    if (delta_time >= kMinDeltaTimeForInertia) {
        const vne::math::Vec3f sample = delta_world / static_cast<float>(delta_time);
        inertia_pan_velocity_ = inertia_pan_velocity_ + (sample - inertia_pan_velocity_) * kPanVelocityBlend;
    }
}

void OrbitArcballBehavior::endPan(double) noexcept {
    interaction_.panning = false;
    if (pivot_mode_ == OrbitPivotMode::eViewCenter && camera_) {
        coi_world_ = camera_->getTarget();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        onPivotChanged();
    }
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::onZoomDolly(float factor, float mouse_x_px, float mouse_y_px) noexcept {
    if (!camera_) {
        return;
    }
    // Apply zoom_speed_ so setZoomSpeed() affects dolly (e.g. pow(factor, zoom_speed_) for perspective).
    const float effective_factor = std::pow(factor, zoom_speed_);
    // Ortho: delegate to shared cursor-anchored zoom, then sync pivot.
    if (auto ortho = orthoCamera()) {
        applyOrthoZoomToCursor(effective_factor, mouse_x_px, mouse_y_px);
        coi_world_ = ortho->getTarget();
        return;
    }
    // Perspective: dolly orbit distance + cursor shift.
    const float old_dist = orbit_distance_;
    orbit_distance_ = vne::math::clamp(orbit_distance_ * effective_factor, kMinOrbitDistance, kMaxOrbitDistance);
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    if (perspCamera() && viewport().width > 0.0f && viewport().height > 0.0f) {
        const vne::math::Vec3f cursor_world =
            worldUnderCursor(*camera_, mouse_x_px, mouse_y_px, old_dist, viewport());
        const vne::math::Vec2f ndc = mouseToNDC(mouse_x_px, mouse_y_px, viewport().width, viewport().height);
        const float fov_y_rad = vne::math::degToRad(perspCamera()->getFieldOfView());
        const float new_half_h = orbit_distance_ * vne::math::tan(fov_y_rad * 0.5f);
        const float new_half_w = new_half_h * (viewport().width / viewport().height);
        coi_world_ = cursor_world - r * (ndc.x() * new_half_w) - u * (ndc.y() * new_half_h);
    }
    applyToCamera();
    if (pivot_mode_ == OrbitPivotMode::eViewCenter) {
        onPivotChanged();
    }
}

// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::doPanInertia(double delta_time) noexcept {
    if (delta_time <= 0.0 || !camera_) {
        return;
    }
    if (inertia_pan_velocity_.length() <= kInertiaPanThreshold) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
    inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);

    if (pivot_mode_ == OrbitPivotMode::eFixed) {
        camera_->setPosition(camera_->getPosition() + delta);
        camera_->setTarget(camera_->getTarget() + delta);
        camera_->updateMatrices();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    } else {
        coi_world_ += delta;
        applyToCamera();
    }
}

void OrbitArcballBehavior::applyInertia(double delta_time) noexcept {
    if (!camera_ || delta_time <= 0.0) {
        return;
    }
    const float dt = static_cast<float>(delta_time);

    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        bool rotation_applied = false;
        bool changed = false;
        vne::math::Vec3f pan_delta_fixed(0.0f, 0.0f, 0.0f);
        if (std::abs(inertia_rot_speed_) > kInertiaRotSpeedThreshold) {
            const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(inertia_rot_axis_, inertia_rot_speed_ * dt);
            orientation_ = (q * orientation_).normalized();
            inertia_rot_speed_ = vne::math::damp(inertia_rot_speed_, 0.0f, 1.0f / rot_damping_, dt);
            rotation_applied = true;
            changed = true;
        }
        if (inertia_pan_velocity_.length() > kInertiaPanSpeedThreshold) {
            const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
            inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
            if (pivot_mode_ == OrbitPivotMode::eFixed) {
                pan_delta_fixed = delta;
                // Apply pan after applyToCamera() when rotation also ran, so we don't overwrite orientation
            } else {
                coi_world_ += delta;
                changed = true;
            }
        }
        if (rotation_applied || changed) {
            applyToCamera();
        }
        if (pan_delta_fixed.length() > kEpsilon) {
            const vne::math::Vec3f new_eye = camera_->getPosition() + pan_delta_fixed;
            const vne::math::Vec3f new_target = camera_->getTarget() + pan_delta_fixed;
            camera_->lookAt(new_eye, new_target, camera_->getUp());
            camera_->updateMatrices();
            orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        }
    } else {
        // Euler rotation inertia
        if (std::abs(inertia_rot_speed_x_) > kInertiaRotThreshold
            || std::abs(inertia_rot_speed_y_) > kInertiaRotThreshold) {
            yaw_deg_ += inertia_rot_speed_x_ * dt;
            pitch_deg_ += inertia_rot_speed_y_ * dt;
            pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
            inertia_rot_speed_x_ = vne::math::damp(inertia_rot_speed_x_, 0.0f, 1.0f / rot_damping_, dt);
            inertia_rot_speed_y_ = vne::math::damp(inertia_rot_speed_y_, 0.0f, 1.0f / rot_damping_, dt);
            applyToCamera();
        }
        doPanInertia(delta_time);
    }
}

// ---------------------------------------------------------------------------
// fitToAABB
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    const vne::math::Vec3f extents = max_world - min_world;
    float radius = extents.length() * 0.5f;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }

    target_coi_world_ = center;
    coi_world_ = center;

    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float aspect = std::max(viewport().width / viewport().height, kMinOrthoExtent);
        const float fov_x_rad = 2.0f * vne::math::atan(vne::math::tan(fov_y_rad * 0.5f) * aspect);
        const float dist_y = radius / vne::math::tan(fov_y_rad * 0.5f);
        const float dist_x = radius / vne::math::tan(fov_x_rad * 0.5f);
        target_orbit_distance_ = std::max(dist_x, dist_y) * kFitToAabbMargin;
        orbit_distance_ = target_orbit_distance_;
        applyToCamera();
        onPivotChanged();
        animating_fit_ = true;
    } else if (auto ortho = orthoCamera()) {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f r = computeRight(front);
        const vne::math::Vec3f u = computeUp(front, r);
        const vne::math::Vec3f corners[8] = {
            min_world,
            {max_world.x(), min_world.y(), min_world.z()},
            {min_world.x(), max_world.y(), min_world.z()},
            {min_world.x(), min_world.y(), max_world.z()},
            {max_world.x(), max_world.y(), min_world.z()},
            {max_world.x(), min_world.y(), max_world.z()},
            {min_world.x(), max_world.y(), max_world.z()},
            max_world,
        };
        float max_r = 0.0f;
        float max_u = 0.0f;
        for (const auto& c : corners) {
            const vne::math::Vec3f d = c - center;
            max_r = std::max(max_r, std::abs(d.dot(r)));
            max_u = std::max(max_u, std::abs(d.dot(u)));
        }
        max_r = std::max(max_r * kFitToAabbMargin, kMinOrthoExtent);
        max_u = std::max(max_u * kFitToAabbMargin, kMinOrthoExtent);
        const float aspect = viewport().width / viewport().height;
        if (max_r / max_u < aspect) {
            max_r = max_u * aspect;
        } else {
            max_u = max_r / aspect;
        }
        ortho->setBounds(-max_r, max_r, -max_u, max_u, ortho->getNearPlane(), ortho->getFarPlane());
        coi_world_ = center;
        target_coi_world_ = center;
        applyToCamera();
        onPivotChanged();
    }
}

// ---------------------------------------------------------------------------
// getWorldUnitsPerPixel
// ---------------------------------------------------------------------------

float OrbitArcballBehavior::getWorldUnitsPerPixel() const noexcept {
    if (auto ortho = orthoCamera()) {
        return ortho->getHeight() / viewport().height;
    }
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * orbit_distance_ * vne::math::tan(fov_y_rad * 0.5f) / viewport().height;
    }
    return 0.0f;
}

// ---------------------------------------------------------------------------
// resetState
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::resetState() noexcept {
    interaction_.rotating = false;
    interaction_.panning = false;
    interaction_.modifier_shift = false;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
    inertia_rot_speed_ = 0.0f;
    inertia_rot_axis_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    arcball_start_x_ = 0.0f;
    arcball_start_y_ = 0.0f;
    normalize_counter_ = 0;
    animating_fit_ = false;
    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        syncFromCamera();
    }
}

// ---------------------------------------------------------------------------
// Public setters that need logic
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::setWorldUp(const vne::math::Vec3f& world_up) noexcept {
    if (world_up.length() < kEpsilon) {
        return;
    }
    world_up_ = world_up.normalized();
}

void OrbitArcballBehavior::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = vne::math::clamp(distance, kMinOrbitDistance, kMaxOrbitDistance);
    applyToCamera();
}

void OrbitArcballBehavior::setPivot(const vne::math::Vec3f& pos, CenterOfInterestSpace space) noexcept {
    if (!camera_) {
        coi_world_ = pos;
        return;
    }
    if (space == CenterOfInterestSpace::eWorldSpace) {
        coi_world_ = pos;
    } else {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f r = computeRight(front);
        const vne::math::Vec3f u = computeUp(front, r);
        coi_world_ = camera_->getPosition() + r * pos.x() + u * pos.y() + front * pos.z();
    }
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
    // Note: pivot_mode_ is intentionally NOT reset here — callers control
    // the mode independently (e.g. InspectController::setPivot sets eFixed).
    syncFromCamera();
}

void OrbitArcballBehavior::setLandmark(const vne::math::Vec3f& world_pos) noexcept {
    coi_world_ = world_pos;
    pivot_mode_ = OrbitPivotMode::eFixed;
    if (camera_) {
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        camera_->setTarget(coi_world_);
        camera_->updateMatrices();
    }
}

void OrbitArcballBehavior::setViewDirection(ViewDirection dir) noexcept {
    // Shared yaw/pitch table — pitch is the look-direction elevation, so negative
    // means looking down (camera above), positive means looking up (camera below).
    float yaw = 0.0f;
    float pitch = 0.0f;
    switch (dir) {
        case ViewDirection::eFront:
            yaw = 0.0f;
            pitch = 0.0f;
            break;
        case ViewDirection::eBack:
            yaw = 180.0f;
            pitch = 0.0f;
            break;
        case ViewDirection::eLeft:
            yaw = -90.0f;
            pitch = 0.0f;
            break;
        case ViewDirection::eRight:
            yaw = 90.0f;
            pitch = 0.0f;
            break;
        case ViewDirection::eTop:
            yaw = 0.0f;
            pitch = -89.0f;
            break;
        case ViewDirection::eBottom:
            yaw = 0.0f;
            pitch = 89.0f;
            break;
        case ViewDirection::eIso:
            yaw = 45.0f;
            pitch = -35.2643897f;
            break;
    }

    yaw_deg_ = yaw;
    pitch_deg_ = pitch;

    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        // Apply via Euler path temporarily, then bake the result into orientation_.
        const OrbitRotationMode prev = rotation_mode_;
        rotation_mode_ = OrbitRotationMode::eOrbit;
        applyToCamera();
        rotation_mode_ = prev;
        syncFromCamera();
    } else {
        applyToCamera();
    }
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::onUpdate(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    // Smooth fitToAABB animation
    if (animating_fit_ && delta_time > 0.0) {
        const float alpha = 1.0f - std::exp(-kFitAnimationSpeed * static_cast<float>(delta_time));
        orbit_distance_ += (target_orbit_distance_ - orbit_distance_) * alpha;
        coi_world_ = coi_world_ + (target_coi_world_ - coi_world_) * alpha;
        applyToCamera();
        const float dist_diff = std::abs(orbit_distance_ - target_orbit_distance_);
        const float coi_diff = (coi_world_ - target_coi_world_).length();
        if (dist_diff < kFitConvergeThreshold && coi_diff < kFitConvergeThreshold) {
            orbit_distance_ = target_orbit_distance_;
            coi_world_ = target_coi_world_;
            applyToCamera();
            animating_fit_ = false;
            onPivotChanged();
        }
    }
    if (!animating_fit_ && !interaction_.rotating && !interaction_.panning) {
        applyInertia(delta_time);
    }
}

// ---------------------------------------------------------------------------
// onAction
// ---------------------------------------------------------------------------

bool OrbitArcballBehavior::onAction(CameraActionType action,
                                    const CameraCommandPayload& payload,
                                    double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eBeginRotate:
            beginRotate(payload.x_px, payload.y_px);
            return true;

        case CameraActionType::eRotateDelta:
            if (interaction_.rotating) {
                if (rotation_mode_ == OrbitRotationMode::eArcball) {
                    // Arcball needs the running absolute screen position.
                    // The payload.x_px / y_px carry absolute cursor position;
                    // update arcball_start_ was already set at beginRotate, so
                    // we always pass the current cursor pos (dragRotateArcball
                    // updates arcball_start_ internally).
                    dragRotateArcball(payload.x_px, payload.y_px, delta_time);
                } else {
                    dragRotateEuler(payload.delta_x_px, payload.delta_y_px, delta_time);
                }
                return true;
            }
            return false;

        case CameraActionType::eEndRotate:
            endRotate(delta_time);
            return true;

        case CameraActionType::eBeginPan:
            beginPan(payload.x_px, payload.y_px);
            return true;

        case CameraActionType::ePanDelta:
            if (interaction_.panning) {
                dragPan(payload.x_px, payload.y_px, payload.delta_x_px, payload.delta_y_px, delta_time);
                interaction_.last_x_px = payload.x_px;
                interaction_.last_y_px = payload.y_px;
                return true;
            }
            return false;

        case CameraActionType::eEndPan:
            endPan(delta_time);
            return true;

        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f && payload.zoom_factor != 1.0f) {
                dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px);
                return true;
            }
            VNE_LOG_DEBUG << "OrbitArcballBehavior: ignoring zoom with factor=" << payload.zoom_factor;
            return false;

        case CameraActionType::eOrbitPanModifier:
            interaction_.modifier_shift = payload.pressed;
            return true;

        case CameraActionType::eResetView:
            resetState();
            return true;

        case CameraActionType::eSetPivotAtCursor: {
            // Move orbit pivot to camera_pos + view_dir * orbit_distance_
            const vne::math::Vec3f front = computeFront();
            coi_world_ = camera_->getPosition() + front * orbit_distance_;
            orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
            pivot_mode_ = OrbitPivotMode::eCoi;
            camera_->setTarget(coi_world_);
            camera_->updateMatrices();
            onPivotChanged();
            return true;
        }

        default:
            return false;
    }
}

}  // namespace vne::interaction
