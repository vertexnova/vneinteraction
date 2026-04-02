#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file trackball_strategy.h
 * @brief Virtual-trackball (quaternion) rotation strategy for OrbitalCameraManipulator.
 *
 * Wraps TrackballBehavior and owns the orientation quaternion, drag-start snapshot,
 * and trackball-specific inertia (angular axis/speed).
 * Internal header — not installed, not part of the public API.
 */

#include "rotation_strategy.h"

#include "trackball_behavior.h"

#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/math/easing.h>

#include <cmath>

namespace vne::interaction {

/**
 * @brief Virtual-trackball rotation strategy.
 *
 * Owns orientation_, orientation_at_drag_start_, and trackball inertia state.
 * The scaleTrackballDeltaQuaternion helper from orbital_camera_manipulator.cpp
 * is reproduced here to keep all trackball logic self-contained.
 */
class TrackballStrategy final : public IRotationStrategy {
   public:
    TrackballStrategy() noexcept
        : orientation_(0.0f, 0.0f, 0.0f, 1.0f)
        , orientation_at_drag_start_(0.0f, 0.0f, 0.0f, 1.0f) {}

    void beginRotate(float x_px, float y_px, OrbitalContext& ctx) noexcept override {
        trackball_.setViewport(vne::math::Vec2f(ctx.viewport.width, ctx.viewport.height));
        trackball_.setGraphicsApi(ctx.graphics_api);
        syncFromCamera(ctx);
        trackball_.beginDrag(vne::math::Vec2f(x_px, y_px));
        orientation_at_drag_start_ = orientation_;
        inertia_rot_speed_ = 0.0f;
    }

    void dragRotate(float x_px,
                    float y_px,
                    float /*delta_x_px*/,
                    float /*delta_y_px*/,
                    double delta_time,
                    OrbitalContext& ctx) noexcept override {
        trackball_.setViewport(vne::math::Vec2f(ctx.viewport.width, ctx.viewport.height));
        trackball_.setGraphicsApi(ctx.graphics_api);

        const vne::math::Vec2f cursor(x_px, y_px);
        const vne::math::Vec3f prev_sphere = trackball_.previousOnSphere();
        const vne::math::Vec3f curr_sphere = trackball_.project(cursor);

        const float trackball_rot = rotation_speed_ * trackball_rotation_scale_;
        vne::math::Quatf delta_q = scaleQuaternion(trackball_.cumulativeDeltaQuaternion(cursor), trackball_rot);
        orientation_ = (orientation_at_drag_start_ * delta_q.conjugate()).normalized();

        updateInertia(prev_sphere, curr_sphere, trackball_rot, delta_time);
        trackball_.endFrame(cursor);
    }

    void endRotate(bool inertia_enabled, double /*dt*/) noexcept override {
        if (!inertia_enabled) {
            inertia_rot_speed_ = 0.0f;
        }
    }

    bool stepInertia(float dt, float damping, OrbitalContext& /*ctx*/) noexcept override {
        constexpr float kThreshold = 1e-4f;
        constexpr float kSpeedMax = 10.0f;
        (void)kSpeedMax;
        if (std::abs(inertia_rot_speed_) <= kThreshold) {
            return false;
        }
        const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(inertia_rot_axis_, inertia_rot_speed_ * dt);
        orientation_ = (q * orientation_).normalized();
        inertia_rot_speed_ = vne::math::damp(inertia_rot_speed_, 0.0f, 1.0f / damping, dt);
        normalize_counter_++;
        if (normalize_counter_ >= 64) {
            orientation_ = orientation_.normalized();
            normalize_counter_ = 0;
        }
        return true;
    }

    void syncFromCamera(OrbitalContext& ctx) noexcept override {
        if (!ctx.camera) {
            return;
        }
        vne::math::Vec3f back = ctx.camera->getPosition() - ctx.coi_world;
        const float back_len = back.length();
        back = (back_len < 1e-6f) ? vne::math::Vec3f(0.0f, 0.0f, 1.0f) : (back / back_len);

        vne::math::Vec3f up = ctx.camera->getUp();
        const float up_len = up.length();
        up = (up_len < 1e-6f) ? ctx.world_up : (up / up_len);

        vne::math::Vec3f right = up.cross(back);
        const float right_len = right.length();
        if (right_len < 1e-6f) {
            right = ctx.world_up.cross(back);
            const float r2 = right.length();
            right = (r2 < 1e-6f) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (right / r2);
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
    }

    [[nodiscard]] vne::math::Vec3f computeBackDirection(OrbitalContext& /*ctx*/) const noexcept override {
        return orientation_.getZAxis();
    }

    [[nodiscard]] vne::math::Vec3f computeUpHint(OrbitalContext& /*ctx*/) const noexcept override {
        return orientation_.getYAxis();
    }

    void applyViewDirection(float yaw_deg, float pitch_deg, OrbitalContext& ctx) noexcept override {
        // Build Euler pose to derive camera position, then re-sync orientation_ from that.
        // Temporarily delegate to orbit_behavior logic via an inline computation.
        // We reuse OrbitBehavior to derive the front, apply to camera, then sync back.
        // This avoids duplicating the Euler-to-direction math here.
        euler_for_preset_.setYawPitch(yaw_deg, pitch_deg);
        const vne::math::Vec3f front = euler_for_preset_.computeFrontDirection(ctx.world_up);
        // Compute camera basis from front
        vne::math::Vec3f r = front.cross(ctx.world_up);
        const float rlen = r.length();
        if (rlen >= 1e-6f) {
            r /= rlen;
        } else {
            r = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
        }
        const vne::math::Vec3f u_raw = r.cross(front);
        const float ulen = u_raw.length();
        const vne::math::Vec3f u = (ulen >= 1e-6f) ? (u_raw / ulen) : ctx.world_up;
        const vne::math::Vec3f back = -front;

        const vne::math::Mat4f rot(vne::math::Vec4f(r.x(), r.y(), r.z(), 0.0f),
                                   vne::math::Vec4f(u.x(), u.y(), u.z(), 0.0f),
                                   vne::math::Vec4f(back.x(), back.y(), back.z(), 0.0f),
                                   vne::math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
        orientation_ = vne::math::Quatf(rot).normalized();
        normalize_counter_ = 0;
    }

    void setRotationSpeed(float speed) noexcept override { rotation_speed_ = speed; }
    void clearInertia() noexcept override {
        inertia_rot_speed_ = 0.0f;
        trackball_.reset();
    }
    void reset(OrbitalContext& ctx) noexcept override {
        clearInertia();
        normalize_counter_ = 0;
        syncFromCamera(ctx);
    }

    // ---- Trackball-only accessors for OrbitalCameraManipulator ------------------

    [[nodiscard]] TrackballBehavior& trackball() noexcept { return trackball_; }
    [[nodiscard]] const TrackballBehavior& trackball() const noexcept { return trackball_; }
    [[nodiscard]] vne::math::Quatf orientation() const noexcept { return orientation_; }

    void setTrackballRotationScale(float scale) noexcept { trackball_rotation_scale_ = scale; }
    [[nodiscard]] float getTrackballRotationScale() const noexcept { return trackball_rotation_scale_; }

   private:
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

    void updateInertia(const vne::math::Vec3f& prev_sphere,
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
        const vne::math::Vec3f r = orientation_.getXAxis();
        const vne::math::Vec3f u = orientation_.getYAxis();
        const vne::math::Vec3f b = orientation_.getZAxis();
        inertia_rot_axis_ = (r * fd.axis_ball.x() - u * fd.axis_ball.y() + b * fd.axis_ball.z()).normalized();
        inertia_rot_speed_ =
            vne::math::clamp((fd.angle_rad * trackball_rot) / static_cast<float>(delta_time), -kSpeedMax, kSpeedMax);
    }

    TrackballBehavior trackball_;
    vne::math::Quatf orientation_;
    vne::math::Quatf orientation_at_drag_start_;
    vne::math::Vec3f inertia_rot_axis_{0.0f, 1.0f, 0.0f};
    float inertia_rot_speed_ = 0.0f;
    uint32_t normalize_counter_ = 0;
    float rotation_speed_ = 0.2f;
    float trackball_rotation_scale_ = 2.5f;

    // Used only for view-direction preset baking (applyViewDirection)
    OrbitBehavior euler_for_preset_;
};

}  // namespace vne::interaction
