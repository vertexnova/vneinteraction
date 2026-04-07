/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "interaction_utils.h"
#include "detail/trackball_behavior.h"

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
// Internal trackball rotation (no virtual table; lives only in this TU)
// ---------------------------------------------------------------------------

struct OrbitalTrackballRotation {
    static constexpr float kVectorEpsilon = 1e-6f;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers) — periodic quat renormalize cadence
    static constexpr int kOrientationRenormalizePeriod = 64;

    TrackballBehavior trackball;
    vne::math::Quatf orientation{0.0f, 0.0f, 0.0f, 1.0f};
    vne::math::Quatf orientation_at_drag_start{0.0f, 0.0f, 0.0f, 1.0f};
    vne::math::Vec3f inertia_rot_axis{0.0f, 1.0f, 0.0f};
    float inertia_rot_speed = 0.0f;
    int normalize_counter = 0;

    [[nodiscard]] static vne::math::Quatf scaleQuaternion(vne::math::Quatf q, float scale) noexcept {
        if (scale <= 0.0f) {
            return vne::math::Quatf::identity();
        }
        if (q.w < 0.0f) {
            q = vne::math::Quatf(-q.x, -q.y, -q.z, -q.w);
        }
        const float imag_sq = q.x * q.x + q.y * q.y + q.z * q.z;
        constexpr float kImagEpsSq = 1e-12f;
        if (imag_sq < kImagEpsSq) {
            if (scale <= 1.0f) {
                return vne::math::Quatf::slerp(vne::math::Quatf::identity(), q, scale);
            }
            if (imag_sq <= 0.0f) {
                return vne::math::Quatf::identity();
            }
        }
        const float ang = q.angle();
        const vne::math::Vec3f axis =
            vne::math::Vec3f(q.x, q.y, q.z) * (1.0f / std::sqrt(std::max(imag_sq, kImagEpsSq)));
        return vne::math::Quatf::fromAxisAngle(axis, ang * scale);
    }

    void updateInertiaFromSpheres(const vne::math::Vec3f& prev_sphere,
                                  const vne::math::Vec3f& curr_sphere,
                                  float trackball_rot,
                                  double delta_time) noexcept {
        constexpr double kMinDt = 0.001;
        constexpr float kAngleThreshold = 1e-6f;
        constexpr float kSpeedMax = 10.0f;
        const BallFrameDelta fd = TrackballBehavior::ballFrameDeltaFromSpheres(prev_sphere, curr_sphere);
        if (delta_time < kMinDt || !fd.valid || fd.angle_rad <= kAngleThreshold) {
            return;
        }
        const vne::math::Vec3f r = orientation.getXAxis();
        const vne::math::Vec3f u = orientation.getYAxis();
        const vne::math::Vec3f b = orientation.getZAxis();
        inertia_rot_axis = (r * fd.axis_ball.x() - u * fd.axis_ball.y() + b * fd.axis_ball.z()).normalized();
        inertia_rot_speed =
            vne::math::clamp((fd.angle_rad * trackball_rot) / static_cast<float>(delta_time), -kSpeedMax, kSpeedMax);
    }

    void syncFromCamera(const std::shared_ptr<vne::scene::ICamera>& camera,
                        const vne::math::Vec3f& coi_world,
                        const vne::math::Vec3f& world_up) noexcept {
        if (!camera) {
            return;
        }
        vne::math::Vec3f back = camera->getPosition() - coi_world;
        const float back_len = back.length();
        if (back_len < kVectorEpsilon) {
            return;  // Degenerate eye–COI (e.g. zero-size fit); keep last valid orientation_.
        }
        back /= back_len;

        vne::math::Vec3f up = camera->getUp();
        const float up_len = up.length();
        up = (up_len < kVectorEpsilon) ? world_up : (up / up_len);

        vne::math::Vec3f right = up.cross(back);
        const float right_len = right.length();
        if (right_len < kVectorEpsilon) {
            right = world_up.cross(back);
            const float r2 = right.length();
            right = (r2 < kVectorEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (right / r2);
        } else {
            right /= right_len;
        }
        up = back.cross(right);

        const vne::math::Mat4f rot(vne::math::Vec4f(right.x(), right.y(), right.z(), 0.0f),
                                   vne::math::Vec4f(up.x(), up.y(), up.z(), 0.0f),
                                   vne::math::Vec4f(back.x(), back.y(), back.z(), 0.0f),
                                   vne::math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
        orientation = vne::math::Quatf(rot).normalized();
        normalize_counter = 0;
    }

    [[nodiscard]] vne::math::Vec3f computeBackDirection() const noexcept { return orientation.getZAxis(); }

    [[nodiscard]] vne::math::Vec3f computeUpHint() const noexcept { return orientation.getYAxis(); }

    void beginRotate(float x_px,
                     float y_px,
                     const std::shared_ptr<vne::scene::ICamera>& camera,
                     const vne::math::Vec3f& coi_world,
                     const vne::math::Vec3f& world_up,
                     const vne::math::Viewport& vp,
                     vne::math::GraphicsApi api) noexcept {
        trackball.setViewport(vne::math::Vec2f(vp.width, vp.height));
        trackball.setGraphicsApi(api);
        syncFromCamera(camera, coi_world, world_up);
        trackball.beginDrag(vne::math::Vec2f(x_px, y_px));
        orientation_at_drag_start = orientation;
        inertia_rot_speed = 0.0f;
    }

    void dragRotate(float x_px,
                    float y_px,
                    double delta_time,
                    float rotation_speed,
                    float trackball_rotation_scale,
                    const vne::math::Viewport& vp,
                    vne::math::GraphicsApi api) noexcept {
        trackball.setViewport(vne::math::Vec2f(vp.width, vp.height));
        trackball.setGraphicsApi(api);

        const vne::math::Vec2f cursor(x_px, y_px);
        const vne::math::Vec3f prev_sphere = trackball.previousOnSphere();
        const vne::math::Vec3f curr_sphere = trackball.project(cursor);

        const float trackball_rot = rotation_speed * trackball_rotation_scale;
        const vne::math::Quatf delta_q = scaleQuaternion(trackball.cumulativeDeltaQuaternion(cursor), trackball_rot);
        orientation = (orientation_at_drag_start * delta_q.conjugate()).normalized();

        updateInertiaFromSpheres(prev_sphere, curr_sphere, trackball_rot, delta_time);
        trackball.endFrame(cursor);
    }

    void endRotate(bool inertia_enabled) noexcept {
        if (!inertia_enabled) {
            inertia_rot_speed = 0.0f;
        }
    }

    bool stepInertia(float dt, float damping) noexcept {
        constexpr float kThreshold = 1e-4f;
        constexpr float kMinDamping = 1e-6f;
        if (std::abs(inertia_rot_speed) <= kThreshold) {
            return false;
        }
        if (!std::isfinite(dt) || dt <= 0.0f || !std::isfinite(damping) || damping <= kMinDamping) {
            inertia_rot_speed = 0.0f;
            return false;
        }
        const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(inertia_rot_axis, inertia_rot_speed * dt);
        orientation = (q * orientation).normalized();
        inertia_rot_speed = vne::math::damp(inertia_rot_speed, 0.0f, 1.0f / damping, dt);
        normalize_counter++;
        if (normalize_counter >= kOrientationRenormalizePeriod) {
            orientation = orientation.normalized();
            normalize_counter = 0;
        }
        return true;
    }

    void applyViewDirection(float yaw_deg, float pitch_deg, const vne::math::Vec3f& world_up_in) noexcept {
        const vne::math::Vec3f up =
            (world_up_in.length() < kVectorEpsilon) ? vne::math::Vec3f(0.0f, 1.0f, 0.0f) : world_up_in.normalized();
        vne::math::Vec3f ref_fwd;
        vne::math::Vec3f ref_right;
        buildReferenceFrame(up, ref_fwd, ref_right);
        const float yaw_rad = vne::math::degToRad(yaw_deg);
        const float pitch_rad = vne::math::degToRad(pitch_deg);
        const float cp = vne::math::cos(pitch_rad);
        vne::math::Vec3f front = (ref_fwd * vne::math::cos(yaw_rad) + ref_right * vne::math::sin(yaw_rad)) * cp
                                 + up * vne::math::sin(pitch_rad);
        const float front_len = front.length();
        front = (front_len < kVectorEpsilon) ? ref_fwd : (front / front_len);
        vne::math::Vec3f r = front.cross(up);
        const float rlen = r.length();
        if (rlen >= kVectorEpsilon) {
            r /= rlen;
        } else {
            r = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
        }
        const vne::math::Vec3f u_raw = r.cross(front);
        const float ulen = u_raw.length();
        const vne::math::Vec3f u = (ulen >= kVectorEpsilon) ? (u_raw / ulen) : up;
        const vne::math::Vec3f back = -front;

        const vne::math::Mat4f rot(vne::math::Vec4f(r.x(), r.y(), r.z(), 0.0f),
                                   vne::math::Vec4f(u.x(), u.y(), u.z(), 0.0f),
                                   vne::math::Vec4f(back.x(), back.y(), back.z(), 0.0f),
                                   vne::math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
        orientation = vne::math::Quatf(rot).normalized();
        normalize_counter = 0;
    }

    void clearInertia() noexcept {
        inertia_rot_speed = 0.0f;
        trackball.reset();
    }

    void reset(const std::shared_ptr<vne::scene::ICamera>& camera,
               const vne::math::Vec3f& coi_world,
               const vne::math::Vec3f& world_up) noexcept {
        clearInertia();
        normalize_counter = 0;
        syncFromCamera(camera, coi_world, world_up);
    }

    void setOrientationQuat(const vne::math::Quatf& q) noexcept {
        orientation = q.normalized();
        normalize_counter = 0;
        inertia_rot_speed = 0.0f;
        inertia_rot_axis = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
        trackball.reset();
    }

    [[nodiscard]] vne::math::Vec3f viewRight() const noexcept {
        vne::math::Vec3f r = orientation.getXAxis();
        const float len = r.length();
        return (len < kVectorEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
    }

    [[nodiscard]] vne::math::Vec3f viewUp() const noexcept {
        vne::math::Vec3f u = orientation.getYAxis();
        const float len = u.length();
        return (len < kVectorEpsilon) ? vne::math::Vec3f(0.0f, 1.0f, 0.0f) : (u / len);
    }

    [[nodiscard]] vne::math::Vec3f viewFront() const noexcept {
        vne::math::Vec3f f = -orientation.getZAxis();
        const float len = f.length();
        return (len < kVectorEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (f / len);
    }
};

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.orbital_camera");
constexpr float kEpsilon = 1e-6f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kMaxOrbitDistance = 1e6f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kPanVelocityBlendRate = 25.0f;  // EMA time-constant reciprocal (1/s) — frame-rate independent
constexpr float kFitAnimationSpeed = 10.0f;
constexpr float kFitConvergeThreshold = 1e-3f;
constexpr float kInertiaPanSpeedThreshold = 1e-4f;
/** Fraction of cursor–COI offset applied per zoom step toward (in) or away from (out) the cursor; 0.5 = 50%. */
constexpr float kZoomToCursorStrength = 0.5f;
/** Minimum @a delta_time (seconds) for pan inertia EMA sampling; skips noisy ultra-short frames. */
constexpr double kMinDeltaTimeForInertia = 0.001;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers) — degenerate cross-product length squared
constexpr float kRightVectorLenSqEpsilon = 1e-12f;
constexpr float kZoomCursorMaxOrbitFactor = 2.0f;
constexpr float kAabbCenterScale = 0.5f;
constexpr float kPerspWorldUnitsScale = 2.0f;
constexpr float kYawDegBack = 180.0f;
constexpr float kYawDegLeft = -90.0f;
constexpr float kYawDegRight = 90.0f;
constexpr float kYawDegIso = 45.0f;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers) — classic iso view pitch (degrees)
constexpr float kPitchDegIso = -35.2643897f;

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
    if (right.lengthSquared() < kRightVectorLenSqEpsilon) {
        right = vne::math::Vec3f(1.0f, 0.0f, 0.0f).cross(view_dir_unit);
    }
    if (right.lengthSquared() < kRightVectorLenSqEpsilon) {
        return {0.0f, 1.0f, 0.0f};
    }
    right = right.normalized();
    return view_dir_unit.cross(right).normalized();
}

}  // namespace

// ---------------------------------------------------------------------------
// Constructor / destructor / move
// ---------------------------------------------------------------------------

OrbitalCameraManipulator::OrbitalCameraManipulator() noexcept
    : orbital_rot_(std::make_unique<OrbitalTrackballRotation>()) {
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

vne::math::Vec3f OrbitalCameraManipulator::computeFront() noexcept {
    return orbital_rot_->viewFront();
}

void OrbitalCameraManipulator::syncCoiAndDistanceFromCamera() noexcept {
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
    orbital_rot_->syncFromCamera(camera_, coi_world_, world_up_);
}

void OrbitalCameraManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f back = orbital_rot_->computeBackDirection();
    const vne::math::Vec3f view_dir = (-back).normalized();
    const vne::math::Vec3f up_hint = orbital_rot_->computeUpHint();
    const vne::math::Vec3f up = stableCameraUpForLookAt(up_hint, view_dir, world_up_);
    camera_->lookAt(coi_world_ + back * orbit_distance_, coi_world_, up);
    camera_->updateMatrices();
}

void OrbitalCameraManipulator::onPivotChanged() noexcept {
    syncFromCamera();
}

// ---------------------------------------------------------------------------
// Rotation
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::beginRotate(float x_px, float y_px) noexcept {
    interaction_.rotating = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    syncFromCamera();
    orbital_rot_->beginRotate(x_px, y_px, camera_, coi_world_, world_up_, viewport(), graphicsApi());
}

void OrbitalCameraManipulator::dragRotate(
    float x_px, float y_px, float /*delta_x_px*/, float /*delta_y_px*/, double delta_time) noexcept {
    orbital_rot_
        ->dragRotate(x_px, y_px, delta_time, rotation_speed_, trackball_rotation_scale_, viewport(), graphicsApi());
    applyToCamera();
}

void OrbitalCameraManipulator::endRotate(double /*delta_time*/) noexcept {
    interaction_.rotating = false;
    orbital_rot_->endRotate(rotation_inertia_enabled_);
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
    constexpr double kMinDt = kMinDeltaTimeForInertia;
    const double dt = delta_time;
    if (!std::isfinite(dt) || dt <= 0.0 || dt < kMinDt) {
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
    const vne::math::Vec3f r = orbital_rot_->viewRight();
    const vne::math::Vec3f u = orbital_rot_->viewUp();
    vne::math::Vec3f delta_world(0.0f, 0.0f, 0.0f);

    const float vw = viewportWidth();
    const float vh = viewportHeight();
    const vne::math::Vec2f ndc_d = mouseWindowDeltaToNDCDelta(delta_x_px, delta_y_px, vw, vh, graphicsApi());

    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float half_h = orbit_distance_ * vne::math::tan(fov_y_rad * 0.5f);
        const float half_w = (vh > 0.0f) ? half_h * (vw / vh) : half_h;
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
    syncFromCamera();
    const float effective_factor = std::pow(factor, zoom_speed_);

    if (orthoCamera()) {
        CameraManipulatorBase::applyOrthoZoomToCursor(effective_factor, mx, my);
        return;
    }
    if (auto persp = perspCamera()) {
        const float old_dist = orbit_distance_;
        orbit_distance_ = vne::math::clamp(orbit_distance_ * effective_factor, kMinOrbitDistance, kMaxOrbitDistance);
        const vne::math::Vec3f r = orbital_rot_->viewRight();
        const vne::math::Vec3f u = orbital_rot_->viewUp();
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
            if (to_cursor.length() < old_dist * kZoomCursorMaxOrbitFactor && pivot_mode_ != OrbitPivotMode::eFixed) {
                coi_world_ += to_cursor * shift_t;
            }
        }
        applyToCamera();
        // Refresh distance/COI from camera without re-deriving quaternion from lookAt (avoids drift).
        syncCoiAndDistanceFromCamera();
    }
}

// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::applyInertia(double delta_time) noexcept {
    if (!camera_ || !std::isfinite(delta_time) || delta_time <= 0.0) {
        return;
    }
    const auto dt = static_cast<float>(delta_time);

    bool rotation_changed = false;
    if (rotation_inertia_enabled_) {
        rotation_changed = orbital_rot_->stepInertia(dt, rot_damping_);
    }

    vne::math::Vec3f pan_delta_fixed(0.0f, 0.0f, 0.0f);
    bool pan_changed = false;
    if (pan_inertia_enabled_ && inertia_pan_velocity_.length() > kInertiaPanSpeedThreshold) {
        const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
        inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
        if (pivot_mode_ == OrbitPivotMode::eFixed) {
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
    const vne::math::Vec3f center = (min_world + max_world) * kAabbCenterScale;
    const vne::math::Vec3f extents = max_world - min_world;
    float radius = extents.length() * kAabbCenterScale;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }

    target_coi_world_ = center;
    coi_world_ = center;

    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float aspect = std::max(viewport().width / viewport().height, CameraManipulatorBase::kMinOrthoExtent);
        const float fov_x_rad = 2.0f * vne::math::atan(vne::math::tan(fov_y_rad * 0.5f) * aspect);
        const float dist_y = radius / vne::math::tan(fov_y_rad * 0.5f);
        const float dist_x = radius / vne::math::tan(fov_x_rad * 0.5f);
        target_orbit_distance_ = std::max(dist_x, dist_y) * kFitToAabbMargin;
        orbit_distance_ = target_orbit_distance_;
        applyToCamera();
        onPivotChanged();
        animating_fit_ = true;
    } else if (auto ortho = orthoCamera()) {
        const vne::math::Vec3f r = orbital_rot_->viewRight();
        const vne::math::Vec3f u = orbital_rot_->viewUp();
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
        max_r = std::max(max_r * kFitToAabbMargin, CameraManipulatorBase::kMinOrthoExtent);
        max_u = std::max(max_u * kFitToAabbMargin, CameraManipulatorBase::kMinOrthoExtent);
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
        return kPerspWorldUnitsScale * orbit_distance_ * vne::math::tan(fov_y_rad * kAabbCenterScale)
               / viewport().height;
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
    orbital_rot_->reset(camera_, coi_world_, world_up_);
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
        const vne::math::Vec3f r = orbital_rot_->viewRight();
        const vne::math::Vec3f u = orbital_rot_->viewUp();
        coi_world_ = camera_->getPosition() + r * pos.x() + u * pos.y() + front * pos.z();
    }
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
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
    float yaw = 0.0f;
    float pitch = 0.0f;
    switch (dir) {
        case ViewDirection::eFront:
            yaw = 0.0f;
            pitch = 0.0f;
            break;
        case ViewDirection::eBack:
            yaw = kYawDegBack;
            pitch = 0.0f;
            break;
        case ViewDirection::eLeft:
            yaw = kYawDegLeft;
            pitch = 0.0f;
            break;
        case ViewDirection::eRight:
            yaw = kYawDegRight;
            pitch = 0.0f;
            break;
        case ViewDirection::eTop: {
            yaw = 0.0f;
            pitch = -89.0f;
            break;
        }
        case ViewDirection::eBottom: {
            yaw = 0.0f;
            pitch = 89.0f;
            break;
        }
        case ViewDirection::eIso:
            yaw = kYawDegIso;
            pitch = kPitchDegIso;
            break;
    }

    orbital_rot_->clearInertia();
    orbital_rot_->applyViewDirection(yaw, pitch, world_up_);
    applyToCamera();
    syncFromCamera();
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::onUpdate(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
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
// Speed / projection accessors
// ---------------------------------------------------------------------------

void OrbitalCameraManipulator::setRotationSpeed(float speed) noexcept {
    rotation_speed_ = std::max(0.0f, speed);
}

void OrbitalCameraManipulator::setTrackballRotationScale(float scale) noexcept {
    trackball_rotation_scale_ = std::max(0.0f, scale);
}

void OrbitalCameraManipulator::setTrackballProjectionMode(TrackballProjectionMode mode) noexcept {
    trackball_projection_mode_ = mode;
    const auto inner = (mode == TrackballProjectionMode::eRim) ? TrackballBehavior::ProjectionMode::eRim
                                                               : TrackballBehavior::ProjectionMode::eHyperbolic;
    orbital_rot_->trackball.setProjectionMode(inner);
}

TrackballProjectionMode OrbitalCameraManipulator::getTrackballProjectionMode() const noexcept {
    return trackball_projection_mode_;
}

vne::math::Quatf OrbitalCameraManipulator::getOrientation() const noexcept {
    return orbital_rot_->orientation;
}

void OrbitalCameraManipulator::setOrientation(const vne::math::Quatf& rotation) noexcept {
    orbital_rot_->setOrientationQuat(rotation);
    applyToCamera();
}

}  // namespace vne::interaction
