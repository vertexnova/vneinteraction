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

#include "trackball_behavior.h"
#include "../view_math.h"

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.trackball_behavior");

constexpr float kMinViewportAxis = 1e-6f;
/** Radius in normalized trackball coordinates (unit ball). */
constexpr float kTrackballRadius = 1.0f;
constexpr float kEpsilonLen = 1e-6f;
constexpr float kCrossLenSqEps = 1e-14f;
constexpr float kAntiParallelDot = 1e-5f;  //!< dot <= -1 + this → treat as 180° for axis fallback

[[nodiscard]] vne::math::Vec3f normalizeOrDefaultZ(const vne::math::Vec3f& v) noexcept {
    const float len = v.length();
    if (len < kEpsilonLen) {
        return {0.0f, 0.0f, 1.0f};
    }
    return v / len;
}
}  // namespace

namespace vne::interaction {

void TrackballBehavior::setViewport(const vne::math::Vec2f& size_px) noexcept {
    viewport_px_ = size_px;
}

vne::math::Vec3f TrackballBehavior::projectHyperbolic(float rx, float ry) const noexcept {
    // Spherical cap + hyperbolic outer: t = R/sqrt(2); z = sqrt(R² - d²) inside, else t²/d.
    const float radius = kTrackballRadius;
    const float t = radius / std::sqrt(2.0f);
    const float d = std::sqrt(rx * rx + ry * ry);
    float rz{};
    if (d < t) {
        rz = std::sqrt(std::max(0.0f, radius * radius - d * d));
    } else {
        rz = (t * t) / std::max(d, kEpsilonLen);
    }
    return normalizeOrDefaultZ(vne::math::Vec3f(rx, ry, rz));
}

vne::math::Vec3f TrackballBehavior::projectRim(float rx, float ry) const noexcept {
    vne::math::Vec3f v;
    const float r2 = rx * rx + ry * ry;
    if (r2 <= 1.0f) {
        v = vne::math::Vec3f(rx, ry, std::sqrt(std::max(0.0f, 1.0f - r2)));
    } else {
        const float inv_len = 1.0f / std::sqrt(r2);
        v = vne::math::Vec3f(rx * inv_len, ry * inv_len, 0.0f);
    }
    return normalizeOrDefaultZ(v);
}

vne::math::Vec3f TrackballBehavior::project(const vne::math::Vec2f& cursor_px) const noexcept {
    const float w = viewport_px_.x();
    const float h = viewport_px_.y();
    if (w <= 0.0f || h <= 0.0f) {
        VNE_LOG_WARN << "TrackballBehavior::project: invalid viewport size (" << w << ", " << h
                     << "), using +Z fallback";
        return {0.0f, 0.0f, 1.0f};
    }
    const float size_min = std::min(w, h);
    const float half_size = 0.5f * size_min;
    if (half_size < kMinViewportAxis) {
        VNE_LOG_WARN << "TrackballBehavior::project: viewport too small for trackball (min axis " << size_min
                     << " px), using +Z fallback";
        return {0.0f, 0.0f, 1.0f};
    }
    const float x_px = cursor_px.x();
    const float y_px = cursor_px.y();
    // NDC from window + graphics API, then scale to the legacy min(viewport) trackball radius (matches frustum
    // pan/zoom).
    const vne::math::Vec2f ndc = mouseWindowToNDC(x_px, y_px, w, h, graphics_api_);
    const float rx = ndc.x() * (w * 0.5f / half_size);
    const float ry = ndc.y() * (h * 0.5f / half_size);

    switch (projection_mode_) {
        case ProjectionMode::eHyperbolic:
            return projectHyperbolic(rx, ry);
        case ProjectionMode::eRim:
            return projectRim(rx, ry);
        default:
            VNE_LOG_WARN << "TrackballBehavior::project: unknown ProjectionMode value "
                         << static_cast<int>(projection_mode_) << ", using +Z fallback";
            return {0.0f, 0.0f, 1.0f};
    }
}

void TrackballBehavior::beginDrag(const vne::math::Vec2f& cursor_px) noexcept {
    drag_start_on_sphere_ = project(cursor_px);
    last_cursor_px_ = cursor_px;
}

BallFrameDelta TrackballBehavior::ballFrameDeltaFromSpheres(const vne::math::Vec3f& prev_sphere_unit,
                                                            const vne::math::Vec3f& curr_sphere_unit) noexcept {
    BallFrameDelta out{};
    const float dot_pc = vne::math::clamp(prev_sphere_unit.dot(curr_sphere_unit), -1.0f, 1.0f);
    out.angle_rad = std::acos(dot_pc);
    const vne::math::Vec3f frame_cross = prev_sphere_unit.cross(curr_sphere_unit);
    const float cross_len_sq = frame_cross.lengthSquared();

    if (cross_len_sq > kCrossLenSqEps) {
        out.axis_ball = frame_cross * (1.0f / std::sqrt(cross_len_sq));
        out.valid = true;
        return out;
    }
    if (dot_pc <= -1.0f + kAntiParallelDot) {
        vne::math::Vec3f a = vne::math::Vec3f(1.0f, 0.0f, 0.0f).cross(prev_sphere_unit);
        if (a.lengthSquared() < kCrossLenSqEps) {
            a = vne::math::Vec3f(0.0f, 1.0f, 0.0f).cross(prev_sphere_unit);
        }
        out.axis_ball = a.normalized();
        out.valid = true;
    }
    return out;
}

vne::math::Quatf TrackballBehavior::rotationBetween(const vne::math::Vec3f& from, const vne::math::Vec3f& to) noexcept {
    // Shortest arc: same as unnormalized (1 + dot, from×to) with anti-parallel fallback — see Quatf::fromToRotation.
    if (from.lengthSquared() < kEpsilonLen * kEpsilonLen || to.lengthSquared() < kEpsilonLen * kEpsilonLen) {
        VNE_LOG_WARN << "TrackballBehavior::rotationBetween: zero-length endpoint, returning identity";
        return vne::math::Quatf::identity();
    }
    const vne::math::Vec3f a = from.normalized();
    const vne::math::Vec3f b = to.normalized();
    return vne::math::Quatf::fromToRotation(a, b);
}

vne::math::Quatf TrackballBehavior::cumulativeDeltaQuaternion(const vne::math::Vec2f& cursor_px) const noexcept {
    return rotationBetween(drag_start_on_sphere_, project(cursor_px));
}

vne::math::Vec3f TrackballBehavior::previousOnSphere() const noexcept {
    return project(last_cursor_px_);
}

void TrackballBehavior::endFrame(const vne::math::Vec2f& cursor_px) noexcept {
    last_cursor_px_ = cursor_px;
}

void TrackballBehavior::reset() noexcept {
    drag_start_on_sphere_ = vne::math::Vec3f(0.0f, 0.0f, 1.0f);
    last_cursor_px_ = {};
}

}  // namespace vne::interaction
