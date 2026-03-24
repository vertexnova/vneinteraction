/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/behavior_utils.h"

namespace vne::interaction {

vne::math::Vec2f mouseToApiScreen(float mx,
                                  float my,
                                  const vne::math::Viewport& vp,
                                  vne::math::GraphicsApi api) noexcept {
    if (!vne::math::screenOriginIsTopLeft(api)) {
        return vne::math::Vec2f(mx, vp.height - my);
    }
    return vne::math::Vec2f(mx, my);
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
