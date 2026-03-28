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

#include "vertexnova/interaction/orbit_behavior.h"

#include "behavior_utils.h"

#include <vertexnova/math/core/math_utils.h>
#include <vertexnova/math/easing.h>
#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.orbit_behavior");

/** Near-zero threshold. */
constexpr float kEpsilon = 1e-6f;
/** Minimum valid interval (seconds) for inertia rate sampling; clamps @a min_delta_time_for_inertia. */
constexpr double kMinDeltaTimeForInertiaFloor = 1e-9;
}  // namespace

namespace vne::interaction {

void OrbitBehavior::setYawPitch(const float yaw_deg, const float pitch_deg) noexcept {
    yaw_deg_ = yaw_deg;
    pitch_deg_ = vne::math::clamp(pitch_deg, pitch_min_deg_, pitch_max_deg_);
}

void OrbitBehavior::setPitchLimits(const float min_deg, const float max_deg) noexcept {
    if (min_deg >= max_deg) {
        VNE_LOG_WARN << "OrbitBehavior::setPitchLimits: invalid range min_deg=" << min_deg << " max_deg=" << max_deg
                     << " (need min < max), ignoring";
        return;
    }
    pitch_min_deg_ = min_deg;
    pitch_max_deg_ = max_deg;
    pitch_deg_ = vne::math::clamp(pitch_deg_, pitch_min_deg_, pitch_max_deg_);
}

vne::math::Vec3f OrbitBehavior::computeFrontDirection(const vne::math::Vec3f& world_up) const noexcept {
    vne::math::Vec3f up = world_up;
    if (up.length() < kEpsilon) {
        VNE_LOG_WARN << "OrbitBehavior::computeFrontDirection: world_up near zero, using (0, 1, 0)";
        up = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    } else {
        up = up.normalized();
    }
    vne::math::Vec3f ref_fwd;
    vne::math::Vec3f ref_right;
    buildReferenceFrame(up, ref_fwd, ref_right);
    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = vne::math::cos(pitch_rad);
    vne::math::Vec3f front =
        (ref_fwd * vne::math::cos(yaw_rad) + ref_right * vne::math::sin(yaw_rad)) * cp + up * vne::math::sin(pitch_rad);
    const float len = front.length();
    return (len < kEpsilon) ? ref_fwd : (front / len);
}

void OrbitBehavior::syncFromViewDirection(const vne::math::Vec3f& world_up,
                                       const vne::math::Vec3f& view_direction_unit) noexcept {
    if (view_direction_unit.lengthSquared() < kEpsilon * kEpsilon) {
        VNE_LOG_WARN << "OrbitBehavior::syncFromViewDirection: view_direction near zero, ignoring";
        return;
    }
    if (world_up.length() < kEpsilon) {
        VNE_LOG_WARN << "OrbitBehavior::syncFromViewDirection: world_up near zero, using (0, 1, 0)";
    }
    const vne::math::Vec3f w_up =
        (world_up.length() < kEpsilon) ? vne::math::Vec3f(0.0f, 1.0f, 0.0f) : world_up.normalized();
    const vne::math::Vec3f view_dir = view_direction_unit.normalized();
    const float up_comp = vne::math::clamp(view_dir.dot(w_up), -1.0f, 1.0f);
    // No pitch clamp here — match legacy syncFromCamera (drag/stepInertia clamp).
    pitch_deg_ = vne::math::radToDeg(vne::math::asin(up_comp));

    const vne::math::Vec3f horiz = view_dir - w_up * up_comp;
    const float horiz_len = horiz.length();
    if (horiz_len < kEpsilon) {
        yaw_deg_ = 0.0f;
        return;
    }
    const vne::math::Vec3f horiz_n = horiz / horiz_len;
    vne::math::Vec3f ref_fwd;
    vne::math::Vec3f ref_right;
    buildReferenceFrame(w_up, ref_fwd, ref_right);
    yaw_deg_ = vne::math::radToDeg(vne::math::atan2(horiz_n.dot(ref_right), horiz_n.dot(ref_fwd)));
}

void OrbitBehavior::beginDrag() noexcept {
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
}

void OrbitBehavior::applyDrag(const float delta_x_px,
                           const float delta_y_px,
                           const float rotation_speed_deg_per_px,
                           const double delta_time,
                           const double min_delta_time_for_inertia) noexcept {
    if (rotation_speed_deg_per_px < 0.0f) {
        VNE_LOG_WARN << "OrbitBehavior::applyDrag: negative rotation_speed_deg_per_px, clamping to 0";
    }
    const float speed = std::max(0.0f, rotation_speed_deg_per_px);
    yaw_deg_ += delta_x_px * speed;
    pitch_deg_ += delta_y_px * speed;
    pitch_deg_ = vne::math::clamp(pitch_deg_, pitch_min_deg_, pitch_max_deg_);

    double min_inertia_dt = min_delta_time_for_inertia;
    if (!std::isfinite(min_inertia_dt)) {
        VNE_LOG_WARN << "OrbitBehavior::applyDrag: min_delta_time_for_inertia not finite, using floor";
        min_inertia_dt = kMinDeltaTimeForInertiaFloor;
    } else if (min_inertia_dt <= 0.0) {
        min_inertia_dt = kMinDeltaTimeForInertiaFloor;
    } else {
        min_inertia_dt = std::max(min_inertia_dt, kMinDeltaTimeForInertiaFloor);
    }

    const double dt = delta_time;
    if (std::isfinite(dt) && dt > 0.0 && dt >= min_inertia_dt) {
        const double inv = 1.0 / dt;
        if (std::isfinite(inv)) {
            const float inv_dt = static_cast<float>(inv);
            inertia_rot_speed_x_ = delta_x_px * speed * inv_dt;
            inertia_rot_speed_y_ = delta_y_px * speed * inv_dt;
        }
    }
}

bool OrbitBehavior::stepInertia(const float delta_time_sec,
                             const float rot_damping,
                             const float inertia_threshold) noexcept {
    if (delta_time_sec <= 0.0f) {
        return false;
    }
    if (rot_damping <= 0.0f) {
        VNE_LOG_WARN << "OrbitBehavior::stepInertia: rot_damping must be > 0, skipping step";
        return false;
    }
    if (std::abs(inertia_rot_speed_x_) <= inertia_threshold && std::abs(inertia_rot_speed_y_) <= inertia_threshold) {
        return false;
    }
    const float dt = delta_time_sec;
    yaw_deg_ += inertia_rot_speed_x_ * dt;
    pitch_deg_ += inertia_rot_speed_y_ * dt;
    pitch_deg_ = vne::math::clamp(pitch_deg_, pitch_min_deg_, pitch_max_deg_);
    inertia_rot_speed_x_ = vne::math::damp(inertia_rot_speed_x_, 0.0f, 1.0f / rot_damping, dt);
    inertia_rot_speed_y_ = vne::math::damp(inertia_rot_speed_y_, 0.0f, 1.0f / rot_damping, dt);
    return true;
}

void OrbitBehavior::clearInertia() noexcept {
    beginDrag();
}

}  // namespace vne::interaction
