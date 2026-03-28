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

#include "vertexnova/interaction/arcball.h"

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kMinViewportAxis = 1e-6f;
/** Radius in normalized trackball coordinates (unit ball). */
constexpr float kTrackballRadius = 1.0f;
constexpr float kEpsilonLen = 1e-6f;

[[nodiscard]] vne::math::Vec3f normalizeOrDefaultZ(const vne::math::Vec3f& v) noexcept {
    const float len = v.length();
    if (len < kEpsilonLen) {
        return vne::math::Vec3f(0.0f, 0.0f, 1.0f);
    }
    return v / len;
}
}  // namespace

void Arcball::setViewport(const vne::math::Vec2f& size_px) noexcept {
    viewport_px_ = size_px;
}

vne::math::Vec3f Arcball::projectHyperbolic(float rx, float ry) const noexcept {
    // Spherical cap + hyperbolic outer: t = R/sqrt(2); z = sqrt(R² - d²) inside, else t²/d.
    const float radius = kTrackballRadius;
    const float t = radius / std::sqrt(2.0f);
    const float d = std::sqrt(rx * rx + ry * ry);
    float rz;
    if (d < t) {
        rz = std::sqrt(std::max(0.0f, radius * radius - d * d));
    } else {
        rz = (t * t) / std::max(d, kEpsilonLen);
    }
    return normalizeOrDefaultZ(vne::math::Vec3f(rx, ry, rz));
}

vne::math::Vec3f Arcball::projectRim(float rx, float ry) const noexcept {
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

vne::math::Vec3f Arcball::project(const vne::math::Vec2f& cursor_px) const noexcept {
    const float w = viewport_px_.x();
    const float h = viewport_px_.y();
    const float size_min = std::min(w, h);
    const float half_size = 0.5f * size_min;
    if (half_size < kMinViewportAxis) {
        return vne::math::Vec3f(0.0f, 0.0f, 1.0f);
    }
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    const float x_px = cursor_px.x();
    const float y_px = cursor_px.y();
    // Screen Y increases downward: (y_px - center_y).
    const float rx = (x_px - cx) / half_size;
    const float ry = (y_px - cy) / half_size;

    switch (projection_mode_) {
        case ProjectionMode::eHyperbolic:
            return projectHyperbolic(rx, ry);
        case ProjectionMode::eRim:
            return projectRim(rx, ry);
    }
}

void Arcball::beginDrag(const vne::math::Vec2f& cursor_px) noexcept {
    drag_start_on_sphere_ = project(cursor_px);
    last_cursor_px_ = cursor_px;
}

vne::math::Quatf Arcball::rotationBetween(const vne::math::Vec3f& from, const vne::math::Vec3f& to) noexcept {
    // Shortest arc: same as unnormalized (1 + dot, from×to) with anti-parallel fallback — see Quatf::fromToRotation.
    const vne::math::Vec3f a = from.normalized();
    const vne::math::Vec3f b = to.normalized();
    return vne::math::Quatf::fromToRotation(a, b);
}

vne::math::Quatf Arcball::cumulativeDeltaQuaternion(const vne::math::Vec2f& cursor_px) const noexcept {
    return rotationBetween(drag_start_on_sphere_, project(cursor_px));
}

vne::math::Vec3f Arcball::previousOnSphere() const noexcept {
    return project(last_cursor_px_);
}

void Arcball::endFrame(const vne::math::Vec2f& cursor_px) noexcept {
    last_cursor_px_ = cursor_px;
}

void Arcball::reset() noexcept {
    drag_start_on_sphere_ = vne::math::Vec3f(0.0f, 0.0f, 1.0f);
    last_cursor_px_ = {};
}

}  // namespace vne::interaction
