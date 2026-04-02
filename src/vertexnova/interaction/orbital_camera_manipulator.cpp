/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "view_math.h"
#include "detail/euler_orbit_strategy.h"
#include "detail/trackball_strategy.h"
#include "detail/rotation_strategy.h"

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
// Anonymous constants
// ---------------------------------------------------------------------------
namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.orbital_camera");
constexpr float kEpsilon = 1e-6f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kMaxOrbitDistance = 1e6f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kInertiaPanThreshold = 1e-4f;
constexpr float kPanVelocityBlendRate = 25.0f;  // EMA time-constant reciprocal (1/s) — frame-rate independent
constexpr float kFitAnimationSpeed = 10.0f;
constexpr float kFitConvergeThreshold = 1e-3f;
constexpr float kInertiaPanSpeedThreshold = 1e-4f;
/** Fraction of cursor–COI offset applied per zoom step toward (in) or away from (out) the cursor; 0.5 = 50%. */
constexpr float kZoomToCursorStrength = 0.5f;
/** Minimum @a delta_time (seconds) for pan inertia EMA sampling; skips noisy ultra-short frames. */
constexpr double kMinDeltaTimeForInertia = 0.001;
constexpr double kMinDeltaTimeForInertiaFloor = 1e-9;

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

}  // namespace

// Orbit pose model:
// - coi_world_  — look-at / orbit center
// - orbit_distance_ — eye–COI distance along view axis
// - rotation_strategy_ — owns all rotation-mode state (Euler or Trackball)

// ---------------------------------------------------------------------------
// Strategy factory
// ---------------------------------------------------------------------------

static std::unique_ptr<IRotationStrategy> makeStrategy(OrbitalRotationMode mode) {
    if (mode == OrbitalRotationMode::eTrackball) {
        return std::make_unique<TrackballStrategy>();
    }
    return std::make_unique<EulerOrbitStrategy>();
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

OrbitalCameraManipulator::OrbitalCameraManipulator() noexcept
    : rotation_strategy_(makeStrategy(OrbitalRotationMode::eOrbit)) {
    world_up_ = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    coi_world_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

OrbitalCameraManipulator::~OrbitalCameraManipulator() noexcept = default;

OrbitalCameraManipulator::OrbitalCameraManipulator(OrbitalCameraManipulator&&) noexcept = default;

OrbitalCameraManipulator& OrbitalCameraManipulator::operator=(OrbitalCameraManipulator&&) noexcept = default;

// ---------------------------------------------------------------------------
// Camera helpers
// ---------------------------------------------------------------------------

bool OrbitalCameraManipulator::isPerspective() const noexcept {
    return static_cast<bool>(perspCamera());
}

bool OrbitalCameraManipulator::isOrthographic() const noexcept {
    return static_cast<bool>(orthoCamera());
}

// ---------------------------------------------------------------------------
// ICameraManipulator: setCamera / onResize
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraManipulatorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "OrbitalCameraManipulator: camera detached (null camera)";
    }
    syncFromCamera();
}

void OrbitalCameraManipulator::onResize(float width_px, float height_px) noexcept {
    CameraManipulatorBase::onResize(width_px, height_px);
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

vne::math::Vec3f OrbitalCameraManipulator::computeRight(const vne::math::Vec3f& front) const noexcept {
    // front × up = right  (standard RH rule; replaces the old up × front = left)
    vne::math::Vec3f r = front.cross(world_up_);
    float len = r.length();
    if (len < kEpsilon) {
        // front is collinear with world_up — use a fallback up axis
        constexpr float kFrontZNearVertical = 0.999f;
        const vne::math::Vec3f fallback_up = (std::abs(front.z()) < kFrontZNearVertical)
                                                 ? vne::math::Vec3f(0.0f, 0.0f, 1.0f)
                                                 : vne::math::Vec3f(0.0f, 1.0f, 0.0f);
        r = front.cross(fallback_up);
        len = r.length();
    }
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

vne::math::Vec3f OrbitalCameraManipulator::computeUp(const vne::math::Vec3f& front,
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

vne::math::Vec3f OrbitalCameraManipulator::computeFront() noexcept {
    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    return -rotation_strategy_->computeBackDirection(ctx);
}

// ---------------------------------------------------------------------------
// syncFromCamera / applyToCamera
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::syncCoiAndDistanceFromCamera() noexcept {
    // eFixed: pivot is a stable world landmark — do not re-derive COI from the camera target or panning breaks.
    if (pivot_mode_ != OrbitPivotMode::eFixed) {
        coi_world_ = camera_->getTarget();
    }
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
}

void OrbitalCameraManipulator::syncFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    syncCoiAndDistanceFromCamera();
    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    rotation_strategy_->syncFromCamera(ctx);
}

void OrbitalCameraManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    const vne::math::Vec3f back = rotation_strategy_->computeBackDirection(ctx);
    const vne::math::Vec3f view_dir = (-back).normalized();
    const vne::math::Vec3f up_hint = rotation_strategy_->computeUpHint(ctx);
    const vne::math::Vec3f up = stableCameraUpForLookAt(up_hint, view_dir, world_up_);
    camera_->lookAt(coi_world_ + back * orbit_distance_, coi_world_, up);
    camera_->updateMatrices();
}

void OrbitalCameraManipulator::onPivotChanged() noexcept {
    // Trackball orientation must be resync'd after the COI changes; Euler yaw/pitch remain valid.
    if (rotation_mode_ == OrbitalRotationMode::eTrackball) {
        syncFromCamera();
    }
}

// ---------------------------------------------------------------------------
// Rotation
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::beginRotate(float x_px, float y_px) noexcept {
    interaction_.rotating = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    syncFromCamera();
    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    rotation_strategy_->beginRotate(x_px, y_px, ctx);
}

void OrbitalCameraManipulator::dragRotate(
    float x_px, float y_px, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    rotation_strategy_->dragRotate(x_px, y_px, delta_x_px, delta_y_px, delta_time, ctx);
    applyToCamera();
}

void OrbitalCameraManipulator::endRotate(double delta_time) noexcept {
    interaction_.rotating = false;
    rotation_strategy_->endRotate(rotation_inertia_enabled_, delta_time);
}

// ---------------------------------------------------------------------------
// Pan
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::beginPan(float x_px, float y_px) noexcept {
    interaction_.panning = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    syncFromCamera();
}

void OrbitalCameraManipulator::applyPanDeltaWorld(const vne::math::Vec3f& delta_world) noexcept {
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

void OrbitalCameraManipulator::updatePanInertiaFromDragSample(const vne::math::Vec3f& delta_world,
                                                              double delta_time) noexcept {
    if (!pan_inertia_enabled_) {
        return;
    }
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

void OrbitalCameraManipulator::dragPan(
    float /*x*/, float /*y*/, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    vne::math::Vec3f delta_world(0.0f, 0.0f, 0.0f);

    const float vw = viewportWidth();
    const float vh = viewportHeight();
    const vne::math::Vec2f ndc_d = mouseWindowDeltaToNDCDelta(delta_x_px, delta_y_px, vw, vh, graphicsApi());

    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float half_h = orbit_distance_ * vne::math::tan(fov_y_rad * 0.5f);
        const float half_w = (vh > 0.0f) ? half_h * (vw / vh) : half_h;
        // Match view-plane extent at orbit distance; minus NDC deltas keep grab-the-world parity with legacy pixel pan.
        delta_world = r * (-ndc_d.x() * half_w * pan_speed_) + u * (-ndc_d.y() * half_h * pan_speed_);
    } else if (auto ortho = orthoCamera()) {
        const float half_w = ortho->getWidth() * 0.5f;
        const float half_h = ortho->getHeight() * 0.5f;
        delta_world = r * (-ndc_d.x() * half_w * pan_speed_) + u * (-ndc_d.y() * half_h * pan_speed_);
    }

    applyPanDeltaWorld(delta_world);
    updatePanInertiaFromDragSample(delta_world, delta_time);
}

void OrbitalCameraManipulator::endPan(double) noexcept {
    interaction_.panning = false;
    if (!pan_inertia_enabled_) {
        inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    }
    if (pivot_mode_ == OrbitPivotMode::eViewCenter && camera_) {
        coi_world_ = camera_->getTarget();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        onPivotChanged();
    }
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::dispatchZoom(float factor, float mx, float my) noexcept {
    if (!camera_ || factor <= 0.0f || !std::isfinite(factor)) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            CameraManipulatorBase::applySceneScaleZoom(factor);
            return;
        case ZoomMethod::eChangeFov:
            CameraManipulatorBase::applyFovZoom(factor);
            return;
        case ZoomMethod::eDollyToCoi:
            applyDolly(factor, mx, my);
            return;
    }
}

void OrbitalCameraManipulator::applyDolly(float factor, float mx, float my) noexcept {
    if (!camera_) {
        return;
    }
    // syncCoiAndDistanceFromCamera() skips pulling coi_world_ from camera_->getTarget() when pivot is eFixed
    // (after fixed-mode pan, target may differ from the landmark COI).
    syncFromCamera();
    // Do not use CameraManipulatorBase::applyDolly for perspective — base is ortho-only (raw factor).
    // Orbit needs orbit_distance_, COI shift, and applyOrthoZoomToCursor(pow factor).
    const float effective_factor = std::pow(factor, zoom_speed_);

    if (orthoCamera()) {
        CameraManipulatorBase::applyOrthoZoomToCursor(effective_factor, mx, my);
        return;
    }
    if (auto persp = perspCamera()) {
        const float old_dist = orbit_distance_;
        orbit_distance_ = vne::math::clamp(orbit_distance_ * effective_factor, kMinOrbitDistance, kMaxOrbitDistance);
        // computeRight/computeUp use view/front; computeFront() matches applyEulerOrbitToCamera / trackball basis.
        const vne::math::Vec3f front_dir = computeFront();
        const vne::math::Vec3f r = computeRight(front_dir);
        const vne::math::Vec3f u = computeUp(front_dir, r);
        const float vw = viewportWidth();
        const float vh = viewportHeight();
        if (vw > 0.0f && vh > 0.0f) {
            const vne::math::Vec2f ndc = mouseWindowToNDC(mx, my, vw, vh, graphicsApi());
            const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
            const float half_h = old_dist * vne::math::tan(fov_y_rad * 0.5f);
            const float half_w = half_h * (vw / vh);
            const vne::math::Vec3f cursor_world = coi_world_ + r * (ndc.x() * half_w) + u * (ndc.y() * half_h);
            const vne::math::Vec3f to_cursor = cursor_world - coi_world_;
            const float shift_t = (1.0f - effective_factor) * kZoomToCursorStrength;
            if (to_cursor.length() < old_dist * 2.0f && pivot_mode_ != OrbitPivotMode::eFixed) {
                coi_world_ += to_cursor * shift_t;
            }
        }
        applyToCamera();
        // Zoom-to-cursor moves COI (except eFixed); lookAt may adjust up.
        // Euler yaw/pitch stay valid; trackball orientation_ must be resync'd after COI change.
        if (rotation_mode_ == OrbitalRotationMode::eTrackball) {
            syncFromCamera();
        }
    }
}
// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::doPanInertia(double delta_time) noexcept {
    if (!pan_inertia_enabled_) {
        return;
    }
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

void OrbitalCameraManipulator::applyInertia(double delta_time) noexcept {
    if (!camera_ || delta_time <= 0.0) {
        return;
    }
    const float dt = static_cast<float>(delta_time);

    // --- rotation inertia via strategy ---
    bool rotation_changed = false;
    if (rotation_inertia_enabled_) {
        OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
        rotation_changed = rotation_strategy_->stepInertia(dt, rot_damping_, ctx);
    }

    // --- pan inertia ---
    vne::math::Vec3f pan_delta_fixed(0.0f, 0.0f, 0.0f);
    bool pan_changed = false;
    if (pan_inertia_enabled_ && inertia_pan_velocity_.length() > kInertiaPanSpeedThreshold) {
        const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
        inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
        if (pivot_mode_ == OrbitPivotMode::eFixed) {
            // Apply after applyToCamera so rotation doesn't overwrite the eye position
            pan_delta_fixed = delta;
        } else {
            coi_world_ += delta;
            pan_changed = true;
        }
    }

    if (rotation_changed || pan_changed) {
        applyToCamera();
    }

    if (pan_delta_fixed.length() > kEpsilon) {
        const vne::math::Vec3f new_eye = camera_->getPosition() + pan_delta_fixed;
        const vne::math::Vec3f new_target = camera_->getTarget() + pan_delta_fixed;
        camera_->lookAt(new_eye, new_target, camera_->getUp());
        camera_->updateMatrices();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    }
}

// ---------------------------------------------------------------------------
// fitToAABB
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::fitToAABB(const vne::math::Vec3f& min_world,
                                         const vne::math::Vec3f& max_world) noexcept {
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

float OrbitalCameraManipulator::getWorldUnitsPerPixel() const noexcept {
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

void OrbitalCameraManipulator::resetState() noexcept {
    interaction_.rotating = false;
    interaction_.panning = false;
    interaction_.modifier_shift = false;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    animating_fit_ = false;
    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    rotation_strategy_->reset(ctx);
}

// ---------------------------------------------------------------------------
// Public setters that need logic
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::setWorldUp(const vne::math::Vec3f& world_up) noexcept {
    if (world_up.length() < kEpsilon) {
        return;
    }
    world_up_ = world_up.normalized();
}

void OrbitalCameraManipulator::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = vne::math::clamp(distance, kMinOrbitDistance, kMaxOrbitDistance);
    applyToCamera();
}

void OrbitalCameraManipulator::setPivot(const vne::math::Vec3f& pos, CenterOfInterestSpace space) noexcept {
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
    // the mode independently (e.g. Inspect3DController::setPivot sets eFixed).
    syncFromCamera();
}

void OrbitalCameraManipulator::setLandmark(const vne::math::Vec3f& world_pos) noexcept {
    coi_world_ = world_pos;
    pivot_mode_ = OrbitPivotMode::eFixed;
    if (camera_) {
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        camera_->setTarget(coi_world_);
        camera_->updateMatrices();
    }
}

void OrbitalCameraManipulator::setViewDirection(ViewDirection dir) noexcept {
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
        case ViewDirection::eTop: {
            yaw = 0.0f;
            if (rotation_mode_ == OrbitalRotationMode::eOrbit) {
                pitch =
                    static_cast<const EulerOrbitStrategy*>(rotation_strategy_.get())->orbitBehavior().getPitchMinDeg();
            } else {
                pitch = OrbitBehavior::kDefaultPitchMinDeg;
            }
            break;
        }
        case ViewDirection::eBottom: {
            yaw = 0.0f;
            if (rotation_mode_ == OrbitalRotationMode::eOrbit) {
                pitch =
                    static_cast<const EulerOrbitStrategy*>(rotation_strategy_.get())->orbitBehavior().getPitchMaxDeg();
            } else {
                pitch = OrbitBehavior::kDefaultPitchMaxDeg;
            }
            break;
        }
        case ViewDirection::eIso:
            yaw = 45.0f;
            pitch = -35.2643897f;
            break;
    }

    OrbitalContext ctx{camera_, world_up_, coi_world_, orbit_distance_, viewport(), graphicsApi()};
    rotation_strategy_->applyViewDirection(yaw, pitch, ctx);
    applyToCamera();
    if (rotation_mode_ == OrbitalRotationMode::eTrackball) {
        // Bake the applied pose back into the trackball orientation after lookAt stabilization.
        syncFromCamera();
    }
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::onUpdate(double delta_time) noexcept {
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

bool OrbitalCameraManipulator::onAction(CameraActionType action,
                                        const CameraCommandPayload& payload,
                                        double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eBeginRotate:
            if (!rotate_enabled_) {
                return false;
            }
            beginRotate(payload.x_px, payload.y_px);
            return true;

        case CameraActionType::eRotateDelta:
            if (!rotate_enabled_) {
                return false;
            }
            if (interaction_.rotating) {
                dragRotate(payload.x_px, payload.y_px, payload.delta_x_px, payload.delta_y_px, delta_time);
                return true;
            }
            return false;

        case CameraActionType::eEndRotate:
            // Unwind gesture even if rotate_enabled_ flipped mid-drag; begin/delta stay gated.
            if (!interaction_.rotating) {
                return false;
            }
            endRotate(delta_time);
            return true;

        case CameraActionType::eBeginPan:
            if (!pan_enabled_) {
                return false;
            }
            beginPan(payload.x_px, payload.y_px);
            return true;

        case CameraActionType::ePanDelta:
            if (!pan_enabled_) {
                return false;
            }
            if (interaction_.panning) {
                dragPan(payload.x_px, payload.y_px, payload.delta_x_px, payload.delta_y_px, delta_time);
                interaction_.last_x_px = payload.x_px;
                interaction_.last_y_px = payload.y_px;
                return true;
            }
            return false;

        case CameraActionType::eEndPan:
            if (!interaction_.panning) {
                return false;
            }
            endPan(delta_time);
            return true;

        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f && payload.zoom_factor != 1.0f) {
                dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px);
                return true;
            }
            return false;

        case CameraActionType::eOrbitPanModifier:
            interaction_.modifier_shift = payload.pressed;
            return true;

        case CameraActionType::eResetView:
            resetState();
            return true;

        case CameraActionType::eSetPivotAtCursor: {
            // Move orbit pivot to camera_pos + view_dir * orbit_distance_ (payload x_px/y_px ignored; not a screen ray)
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

// ---------------------------------------------------------------------------
// setRotationMode / trackball projection accessors
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::setRotationSpeed(float speed) noexcept {
    rotation_speed_ = std::max(0.0f, speed);
    rotation_strategy_->setRotationSpeed(rotation_speed_);
}

void OrbitalCameraManipulator::setTrackballRotationScale(float scale) noexcept {
    trackball_rotation_scale_ = std::max(0.0f, scale);
    if (rotation_mode_ == OrbitalRotationMode::eTrackball && rotation_strategy_) {
        static_cast<TrackballStrategy*>(rotation_strategy_.get())->setTrackballRotationScale(trackball_rotation_scale_);
    }
}

void OrbitalCameraManipulator::setRotationMode(OrbitalRotationMode mode) noexcept {
    if (mode == rotation_mode_) {
        return;
    }
    const OrbitalRotationMode previous = rotation_mode_;
    rotation_mode_ = mode;
    // Preserve trackball projection when leaving trackball (read *before* replacing strategy); otherwise
    // use the stored setting so setTrackballProjectionMode while in Euler still applies on switch.
    TrackballProjectionMode proj = trackball_projection_mode_;
    if (previous == OrbitalRotationMode::eTrackball && rotation_strategy_) {
        const auto inner = static_cast<TrackballStrategy*>(rotation_strategy_.get())->trackball().getProjectionMode();
        proj = (inner == TrackballBehavior::ProjectionMode::eRim) ? TrackballProjectionMode::eRim
                                                                  : TrackballProjectionMode::eHyperbolic;
    }
    trackball_projection_mode_ = proj;
    rotation_strategy_ = makeStrategy(mode);
    if (mode == OrbitalRotationMode::eTrackball) {
        auto* tb = static_cast<TrackballStrategy*>(rotation_strategy_.get());
        tb->trackball().setProjectionMode(proj == TrackballProjectionMode::eRim
                                              ? TrackballBehavior::ProjectionMode::eRim
                                              : TrackballBehavior::ProjectionMode::eHyperbolic);
        tb->setRotationSpeed(rotation_speed_);
        tb->setTrackballRotationScale(trackball_rotation_scale_);
    } else {
        static_cast<EulerOrbitStrategy*>(rotation_strategy_.get())->setRotationSpeed(rotation_speed_);
    }
    syncFromCamera();
}

void OrbitalCameraManipulator::setTrackballProjectionMode(TrackballProjectionMode mode) noexcept {
    trackball_projection_mode_ = mode;
    if (rotation_mode_ == OrbitalRotationMode::eTrackball && rotation_strategy_) {
        const auto inner = (mode == TrackballProjectionMode::eRim) ? TrackballBehavior::ProjectionMode::eRim
                                                                   : TrackballBehavior::ProjectionMode::eHyperbolic;
        static_cast<TrackballStrategy*>(rotation_strategy_.get())->trackball().setProjectionMode(inner);
    }
}

TrackballProjectionMode OrbitalCameraManipulator::getTrackballProjectionMode() const noexcept {
    return trackball_projection_mode_;
}

}  // namespace vne::interaction
