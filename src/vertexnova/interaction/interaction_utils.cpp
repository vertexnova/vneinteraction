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

#include "interaction_utils.h"

#include <algorithm>
#include <cmath>

namespace vne::interaction {

vne::math::Quatf scaleTrackballQuaternion(vne::math::Quatf q, float scale) noexcept {
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
    const vne::math::Vec3f axis = vne::math::Vec3f(q.x, q.y, q.z) * (1.0f / std::sqrt(std::max(imag_sq, kImagEpsSq)));
    return vne::math::Quatf::fromAxisAngle(axis, ang * scale);
}

vne::math::Vec2f mouseToApiScreen(float mx,
                                  float my,
                                  const vne::math::Viewport& vp,
                                  vne::math::GraphicsApi api) noexcept {
    if (!vne::math::screenOriginIsTopLeft(api)) {
        return {mx, vp.height - my};
    }
    return {mx, my};
}

vne::math::Vec2f mouseWindowToNDC(float mx, float my, float w, float h, vne::math::GraphicsApi api) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return {0.0f, 0.0f};
    }
    const vne::math::Viewport vp(w, h);
    const vne::math::Vec2f api_screen = mouseToApiScreen(mx, my, vp, api);
    return vne::math::screenToNDC(api_screen, vp, api);
}

vne::math::Vec2f mouseWindowDeltaToNDCDelta(const float delta_x_px,
                                            const float delta_y_px,
                                            const float w,
                                            const float h,
                                            const vne::math::GraphicsApi api) noexcept {
    if (w <= 0.0f || h <= 0.0f) {
        return {0.0f, 0.0f};
    }
    const float cx = 0.5f * w;
    const float cy = 0.5f * h;
    const vne::math::Vec2f ndc0 = mouseWindowToNDC(cx, cy, w, h, api);
    const vne::math::Vec2f ndc1 = mouseWindowToNDC(cx + delta_x_px, cy + delta_y_px, w, h, api);
    return {ndc1.x() - ndc0.x(), ndc1.y() - ndc0.y()};
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
