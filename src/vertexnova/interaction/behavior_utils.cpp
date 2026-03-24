/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/behavior_utils.h"

#include <vertexnova/math/core/math_utils.h>

#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
}

void buildReferenceFrame(const vne::math::Vec3f& world_up,
                         vne::math::Vec3f& ref_fwd,
                         vne::math::Vec3f& ref_right) noexcept {
    vne::math::Vec3f candidate =
        (std::abs(world_up.y()) > 0.9f) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : vne::math::Vec3f(0.0f, -1.0f, 0.0f);
    ref_fwd = candidate - world_up * candidate.dot(world_up);
    const float fwd_len = ref_fwd.length();
    ref_fwd = (fwd_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (ref_fwd / fwd_len);
    ref_right = ref_fwd.cross(world_up);
    const float right_len = ref_right.length();
    if (right_len > kEpsilon) {
        ref_right /= right_len;
    } else {
        ref_right = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
    }
}

vne::math::Vec2f mouseToApiScreen(float mx,
                                  float my,
                                  const vne::math::Viewport& vp,
                                  vne::math::GraphicsApi api) noexcept {
    if (!vne::math::screenOriginIsTopLeft(api)) {
        return vne::math::Vec2f(mx, vp.height - my);
    }
    return vne::math::Vec2f(mx, my);
}

vne::math::Vec2f mouseToNDC(float mx, float my, float w, float h) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return vne::math::Vec2f(0.0f, 0.0f);
    }
    return vne::math::Vec2f((2.0f * mx / w) - 1.0f, 1.0f - (2.0f * my / h));
}

vne::math::Vec3f mouseUnproject(
    const vne::scene::ICamera& camera, float mx, float my, float depth, const vne::math::Viewport& vp) noexcept {
    const vne::math::GraphicsApi api = camera.getGraphicsApi();
    const vne::math::Vec2f screen = mouseToApiScreen(mx, my, vp, api);
    const vne::math::Mat4f inv_vp = camera.getViewProjectionMatrix().inverse();
    return vne::math::unproject(vne::math::Vec3f(screen.x(), screen.y(), depth), inv_vp, vp, api);
}

vne::math::Ray mouseToWorldRay(const vne::scene::ICamera& camera,
                               float mx,
                               float my,
                               const vne::math::Viewport& vp) noexcept {
    const vne::math::GraphicsApi api = camera.getGraphicsApi();
    const vne::math::Vec2f screen = mouseToApiScreen(mx, my, vp, api);
    const vne::math::Mat4f inv_vp = camera.getViewProjectionMatrix().inverse();
    return vne::math::screenToWorldRay(screen, inv_vp, vp, camera.getPosition(), api);
}

vne::math::Vec3f worldUnderCursor(
    const vne::scene::ICamera& camera, float mx, float my, float distance, const vne::math::Viewport& vp) noexcept {
    const vne::math::Ray ray = mouseToWorldRay(camera, mx, my, vp);
    return ray.origin() + ray.direction() * distance;
}

vne::math::Vec3f worldUnderCursorOrtho(const vne::scene::OrthographicCamera& ortho, float ndc_x, float ndc_y) noexcept {
    const float half_w = ortho.getWidth() * 0.5f;
    const float half_h = ortho.getHeight() * 0.5f;
    const vne::math::Vec3f target = ortho.getTarget();
    vne::math::Vec3f front = target - ortho.getPosition();
    const float front_len = front.length();
    front = (front_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);
    const vne::math::Vec3f up = ortho.getUp().normalized();
    vne::math::Vec3f r = front.cross(up);
    const float r_len = r.length();
    r = (r_len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / r_len);
    return target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
}

vne::math::Vec3f worldUnderCursorPersp(const vne::scene::PerspectiveCamera& persp,
                                       float viewport_width,
                                       float viewport_height,
                                       float orbit_dist,
                                       float ndc_x,
                                       float ndc_y,
                                       const vne::math::Vec3f& front,
                                       const vne::math::Vec3f& right,
                                       const vne::math::Vec3f& up) noexcept {
    const float fov_y_rad = vne::math::degToRad(persp.getFieldOfView());
    const float half_h = orbit_dist * vne::math::tan(fov_y_rad * 0.5f);
    const float aspect = (viewport_height > 0.0f) ? (viewport_width / viewport_height) : 1.0f;
    const float half_w = half_h * aspect;
    return persp.getPosition() + front * orbit_dist + right * (ndc_x * half_w) + up * (ndc_y * half_h);
}

}  // namespace vne::interaction
