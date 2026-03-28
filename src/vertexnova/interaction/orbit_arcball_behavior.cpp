/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbit_arcball_behavior.h"
#include "vertexnova/interaction/behavior_utils.h"

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
// Anonymous constants (mirrors orbit_style_base.cpp / trackball_manipulator.cpp)
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
constexpr float kPanVelocityBlendRate = 25.0f;  // EMA time-constant reciprocal (1/s) — frame-rate independent
constexpr float kFitAnimationSpeed = 10.0f;
constexpr float kFitConvergeThreshold = 1e-3f;
constexpr double kMinDeltaTimeForInertia = 0.001;
/** Floor (seconds) for inertia sampling interval; avoids invalid compare when constants change. */
constexpr double kMinDeltaTimeForInertiaFloor = 1e-9;
constexpr float kInertiaRotThreshold = 1e-3f;
constexpr float kDefaultOrbitDistance = 5.0f;
constexpr float kInertiaRotSpeedMax = 10.0f;
constexpr float kInertiaRotAngleThreshold = 1e-6f;
constexpr float kInertiaRotSpeedThreshold = 1e-4f;
constexpr float kInertiaPanSpeedThreshold = 1e-4f;
/** Strength of COI shift toward cursor on perspective zoom-to-cursor (0..1). */
constexpr float kZoomToCursorStrength = 0.5f;

/**
 * glm::lookAt degenerates when `up` is parallel to view direction (center - eye). Orthonormal
 * camera bases rarely hit this exactly, but drift or extreme tilts can align them — recover using
 * world_up × view_dir when needed.
 */
[[nodiscard]] vne::math::Vec3f stableCameraUpForLookAt(const vne::math::Vec3f& camera_up_hint,
                                                       const vne::math::Vec3f& view_dir_unit,
                                                       const vne::math::Vec3f& world_up) noexcept {
    const float parallel = std::abs(camera_up_hint.dot(view_dir_unit));
    constexpr float kParallelEps = 1e-3f;
    if (parallel < 1.0f - kParallelEps) {
        vne::math::Vec3f ortho = camera_up_hint - view_dir_unit * camera_up_hint.dot(view_dir_unit);
        const float len = ortho.length();
        if (len > kEpsilon) {
            return ortho / len;
        }
    }
    vne::math::Vec3f right = world_up.cross(view_dir_unit);
    if (right.lengthSquared() < 1e-12f) {
        right = vne::math::Vec3f(1.0f, 0.0f, 0.0f).cross(view_dir_unit);
    }
    if (right.lengthSquared() < 1e-12f) {
        return vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    }
    right = right.normalized();
    return view_dir_unit.cross(right).normalized();
}

/**
 * Apply rotation_speed_ to the arcball delta quaternion. Uses slerp near identity (avoids Quat::axis()
 * fallback when |sin(θ/2)| is tiny) and canonical w >= 0 before measuring angle.
 */
[[nodiscard]] vne::math::Quatf scaleTrackballDeltaQuaternion(vne::math::Quatf q, float rotation_speed) noexcept {
    if (rotation_speed <= 0.0f) {
        return vne::math::Quatf::identity();
    }
    if (q.w < 0.0f) {
        q = vne::math::Quatf(-q.x, -q.y, -q.z, -q.w);
    }
    const float imag_sq = q.x * q.x + q.y * q.y + q.z * q.z;
    constexpr float kImagEpsSq = 1e-12f;
    if (imag_sq < kImagEpsSq) {
        if (rotation_speed <= 1.0f) {
            return vne::math::Quatf::slerp(vne::math::Quatf::identity(), q, rotation_speed);
        }
        if (imag_sq <= 0.0f) {
            return vne::math::Quatf::identity();
        }
        const float ang = q.angle();
        const float inv = 1.0f / std::sqrt(imag_sq);
        return vne::math::Quatf::fromAxisAngle(vne::math::Vec3f(q.x * inv, q.y * inv, q.z * inv), ang * rotation_speed);
    }
    const float ang = q.angle();
    const vne::math::Vec3f axis = vne::math::Vec3f(q.x, q.y, q.z) * (1.0f / std::sqrt(imag_sq));
    return vne::math::Quatf::fromAxisAngle(axis, ang * rotation_speed);
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
    trackball_.setViewport(vne::math::Vec2f(width_px, height_px));
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
    return orbit_behavior_.computeFrontDirection(world_up_);
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
        vne::math::Vec3f front = coi_world_ - camera_->getPosition();
        if (orbit_distance_ < kEpsilon) {
            orbit_distance_ = kDefaultOrbitDistance;
            orbit_behavior_.setYawPitch(0.0f, 0.0f);
            return;
        }
        front /= orbit_distance_;
        orbit_behavior_.syncFromViewDirection(world_up_, front);
    }
}

void OrbitArcballBehavior::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    if (rotation_mode_ == OrbitRotationMode::eArcball) {
        const vne::math::Vec3f back = orientation_.getZAxis();
        const vne::math::Vec3f view_dir = (-back).normalized();
        const vne::math::Vec3f up_hint = orientation_.getYAxis();
        const vne::math::Vec3f up = stableCameraUpForLookAt(up_hint, view_dir, world_up_);
        camera_->lookAt(coi_world_ + back * orbit_distance_, coi_world_, up);
    } else {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f view_dir = front.normalized();
        const vne::math::Vec3f up_hint = computeUp(front, computeRight(front));
        const vne::math::Vec3f up = stableCameraUpForLookAt(up_hint, view_dir, world_up_);
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
    const vne::math::Vec2f vp(viewport().width, viewport().height);
    trackball_.setViewport(vp);
    trackball_.beginDrag(vne::math::Vec2f(x_px, y_px));
    orientation_at_drag_start_ = orientation_;
    orbit_behavior_.beginDrag();
    inertia_rot_speed_ = 0.0f;
}

void OrbitArcballBehavior::dragRotateEuler(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    orbit_behavior_.applyDrag(delta_x_px, delta_y_px, rotation_speed_, delta_time, kMinDeltaTimeForInertia);
    applyToCamera();
}

void OrbitArcballBehavior::updateTrackballDragInertiaFromFrame(const vne::math::Vec3f& prev_sphere,
                                                             const vne::math::Vec3f& curr_sphere,
                                                             const float trackball_rot,
                                                             const double delta_time) noexcept {
    const BallFrameDelta fd = TrackballBehavior::ballFrameDeltaFromSpheres(prev_sphere, curr_sphere);
    if (delta_time < kMinDeltaTimeForInertia || !fd.valid || fd.angle_rad <= kInertiaRotAngleThreshold) {
        return;
    }
    // Trackball (right, screen-down, toward eye) → camera basis (r, u, back); screen-down = -u.
    const vne::math::Vec3f r = orientation_.getXAxis();
    const vne::math::Vec3f u = orientation_.getYAxis();
    const vne::math::Vec3f b = orientation_.getZAxis();
    inertia_rot_axis_ = (r * fd.axis_ball.x() - u * fd.axis_ball.y() + b * fd.axis_ball.z()).normalized();
    inertia_rot_speed_ = vne::math::clamp((fd.angle_rad * trackball_rot) / static_cast<float>(delta_time),
                                          -kInertiaRotSpeedMax,
                                          kInertiaRotSpeedMax);
}

void OrbitArcballBehavior::dragRotateArcball(float x_px, float y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }

    const vne::math::Vec2f vp(viewport().width, viewport().height);
    trackball_.setViewport(vp);

    const vne::math::Vec2f cursor(x_px, y_px);
    const vne::math::Vec3f prev_sphere = trackball_.previousOnSphere();
    const vne::math::Vec3f curr_sphere = trackball_.project(cursor);

    // Arcball scales quaternion angle by rotation_speed_; Euler uses deg/pixel — multiply by
    // trackball_rotation_scale_ so a single rotation_speed feels usable in both modes.
    const float trackball_rot = rotation_speed_ * trackball_rotation_scale_;
    vne::math::Quatf delta_q = scaleTrackballDeltaQuaternion(trackball_.cumulativeDeltaQuaternion(cursor), trackball_rot);

    // Conjugate inverts the shortest-arc delta so pointer motion matches the intended orbit direction.
    orientation_ = (orientation_at_drag_start_ * delta_q.conjugate()).normalized();

    applyToCamera();

    updateTrackballDragInertiaFromFrame(prev_sphere, curr_sphere, trackball_rot, delta_time);

    trackball_.endFrame(cursor);
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

void OrbitArcballBehavior::applyPanDeltaWorld(const vne::math::Vec3f& delta_world) noexcept {
    if (!camera_) {
        return;
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
}

void OrbitArcballBehavior::updatePanInertiaFromDragSample(const vne::math::Vec3f& delta_world,
                                                          double delta_time) noexcept {
    double min_inertia_dt = kMinDeltaTimeForInertia;
    if (!std::isfinite(min_inertia_dt)) {
        min_inertia_dt = kMinDeltaTimeForInertiaFloor;
    } else if (min_inertia_dt <= 0.0) {
        min_inertia_dt = kMinDeltaTimeForInertiaFloor;
    } else {
        min_inertia_dt = std::max(min_inertia_dt, kMinDeltaTimeForInertiaFloor);
    }

    const double dt = delta_time;
    if (!std::isfinite(dt) || dt <= 0.0 || dt < min_inertia_dt) {
        return;
    }
    const double inv = 1.0 / dt;
    if (!std::isfinite(inv)) {
        return;
    }
    const vne::math::Vec3f sample = delta_world * static_cast<float>(inv);
    const float blend = 1.0f - std::exp(-kPanVelocityBlendRate * static_cast<float>(dt));
    inertia_pan_velocity_ = inertia_pan_velocity_ + (sample - inertia_pan_velocity_) * blend;
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

    applyPanDeltaWorld(delta_world);
    updatePanInertiaFromDragSample(delta_world, delta_time);
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

void OrbitArcballBehavior::dispatchZoom(float factor, float mx, float my) noexcept {
    if (!camera_ || factor <= 0.0f || !std::isfinite(factor)) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            CameraBehaviorBase::applySceneScaleZoom(factor);
            return;
        case ZoomMethod::eChangeFov:
        case ZoomMethod::eDollyToCoi:
            onZoomDolly(factor, mx, my);
            return;
    }
}

void OrbitArcballBehavior::onZoomDolly(float factor, float mouse_x_px, float mouse_y_px) noexcept {
    if (!camera_) {
        return;
    }
    // Preserve user zoom direction while scaling step intensity with zoom_speed_.
    // zoom_speed_ > 1.0f makes each zoom step stronger, < 1.0f makes it gentler.
    const float effective_factor = std::pow(factor, zoom_speed_);
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            CameraBehaviorBase::applySceneScaleZoom(factor);
            return;
        case ZoomMethod::eChangeFov: {
            auto persp = perspCamera();
            if (persp) {
                const float fov = persp->getFieldOfView();
                const float new_fov =
                    vne::math::clamp(fov * ((factor < 1.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_),
                                     kFovMinDeg,
                                     kFovMaxDeg);
                persp->setFieldOfView(new_fov);
                persp->updateMatrices();
                // If FOV has not changed (limit reached), fall through to dolly so zoom doesn't feel stuck
                if (new_fov != fov) {
                    return;
                }
            }
            [[fallthrough]];
        }
        case ZoomMethod::eDollyToCoi:
            if (orthoCamera()) {
                CameraBehaviorBase::applyOrthoZoomToCursor(effective_factor, mouse_x_px, mouse_y_px);
                return;
            }
            {
                const float old_dist = orbit_distance_;
                orbit_distance_ =
                    vne::math::clamp(orbit_distance_ * effective_factor, kMinOrbitDistance, kMaxOrbitDistance);
                // computeRight/computeUp expect the view/front direction (not camera-back / +Z).
                // computeFront() matches applyToCamera (Euler yaw/pitch or arcball -orientation_.getZAxis()).
                const vne::math::Vec3f front_dir = computeFront();
                const vne::math::Vec3f r = computeRight(front_dir);
                const vne::math::Vec3f u = computeUp(front_dir, r);
                auto persp = perspCamera();
                const float vw = viewportWidth();
                const float vh = viewportHeight();
                if (persp && vw > 0.0f && vh > 0.0f) {
                    const float ndc_x = (2.0f * mouse_x_px / vw) - 1.0f;
                    const float ndc_y = 1.0f - (2.0f * mouse_y_px / vh);
                    const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
                    const float half_h = old_dist * vne::math::tan(fov_y_rad * 0.5f);
                    const float half_w = half_h * (vw / vh);
                    // Camera is along -front from COI at old_dist; cursor offset in the view plane (r, u).
                    const vne::math::Vec3f cursor_world = coi_world_ + r * (ndc_x * half_w) + u * (ndc_y * half_h);
                    const vne::math::Vec3f to_cursor = cursor_world - coi_world_;
                    const float shift_t = (1.0f - effective_factor) * kZoomToCursorStrength;
                    if (to_cursor.length() < old_dist * 2.0f) {
                        coi_world_ += to_cursor * shift_t;
                    }
                }
                applyToCamera();
                if (pivot_mode_ == OrbitPivotMode::eViewCenter) {
                    onPivotChanged();
                } else if (rotation_mode_ == OrbitRotationMode::eArcball) {
                    // Re-sync orientation_ after COI shift so rotation basis stays valid
                    syncFromCamera();
                }
            }
            return;
    }
}

// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void OrbitArcballBehavior::doPanInertia(double delta_time) noexcept {
    if (!std::isfinite(delta_time) || delta_time <= 0.0 || !camera_) {
        return;
    }
    if (inertia_pan_velocity_.length() <= kInertiaPanThreshold) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
    inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);

    applyPanDeltaWorld(delta);
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
        if (orbit_behavior_.stepInertia(dt, rot_damping_, kInertiaRotThreshold)) {
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
    orbit_behavior_.clearInertia();
    inertia_rot_speed_ = 0.0f;
    inertia_rot_axis_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    trackball_.reset();
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
        // Straight down/up: use current Euler pitch limits (defaults ±89°).
        case ViewDirection::eTop:
            yaw = 0.0f;
            pitch = orbit_behavior_.getPitchMinDeg();
            break;
        case ViewDirection::eBottom:
            yaw = 0.0f;
            pitch = orbit_behavior_.getPitchMaxDeg();
            break;
        case ViewDirection::eIso:
            yaw = 45.0f;
            pitch = -35.2643897f;
            break;
    }

    orbit_behavior_.setYawPitch(yaw, pitch);

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
                    // update trackball_start_ was already set at beginRotate, so
                    // we always pass the current cursor pos (dragRotateArcball
                    // updates trackball_start_ internally).
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
