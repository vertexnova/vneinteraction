/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_behavior_base.h"
#include "vertexnova/interaction/behavior_utils.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kZoomOrthoHalfMin = 1e-3f;
constexpr float kZoomOrthoHalfMax = 1e6f;
}  // namespace

// ---------------------------------------------------------------------------
// ICameraBehavior — overrides that need implementation
// ---------------------------------------------------------------------------

void CameraBehaviorBase::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    if (camera_) {
        zoom_scale_ = vne::math::clamp(camera_->getSceneScale(), kSceneScaleMin, kSceneScaleMax);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void CameraBehaviorBase::setFovZoomSpeed(float speed) noexcept {
    fov_zoom_speed_ = std::max(0.01f, speed);
}

void CameraBehaviorBase::setZoomMethod(ZoomMethod method) noexcept {
    const ZoomMethod prev = zoom_method_;
    zoom_method_ = method;
    if (prev == ZoomMethod::eSceneScale && method != ZoomMethod::eSceneScale && camera_) {
        zoom_scale_ = 1.0f;
        camera_->setSceneScale(1.0f);
        camera_->updateMatrices();
    }
}

// ---------------------------------------------------------------------------
// Camera type helpers
// ---------------------------------------------------------------------------

std::shared_ptr<vne::scene::PerspectiveCamera> CameraBehaviorBase::perspCamera() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_);
}

std::shared_ptr<vne::scene::OrthographicCamera> CameraBehaviorBase::orthoCamera() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
}

vne::math::GraphicsApi CameraBehaviorBase::graphicsApi() const noexcept {
    return camera_ ? camera_->getGraphicsApi() : vne::math::GraphicsApi::eOpenGL;
}

// ---------------------------------------------------------------------------
// Zoom dispatch
// ---------------------------------------------------------------------------

void CameraBehaviorBase::dispatchZoom(float factor, float mx, float my) noexcept {
    if (!camera_ || factor <= 0.0f || !std::isfinite(factor)) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eChangeFov:
            applyFovZoom(factor);
            return;
        case ZoomMethod::eSceneScale:
            applySceneScaleZoom(factor);
            return;
        case ZoomMethod::eDollyToCoi:
            applyDolly(factor, mx, my);
            return;
    }
}

void CameraBehaviorBase::applyDolly(float factor, float mx, float my) noexcept {
    // Default: handle ortho cursor-anchored zoom; persp no-op (behaviors override).
    if (orthoCamera()) {
        applyOrthoZoomToCursor(factor, mx, my);
    }
}

// ---------------------------------------------------------------------------
// applyFovZoom
// ---------------------------------------------------------------------------

void CameraBehaviorBase::applyFovZoom(float factor) noexcept {
    if (!camera_ || factor <= 0.0f || !std::isfinite(factor)) {
        return;
    }
    // Use scroll/pinch factor magnitude directly (same convention as dolly ortho + InputMapper).
    if (auto persp = perspCamera()) {
        persp->setFieldOfView(vne::math::clamp(persp->getFieldOfView() * factor, kFovMinDeg, kFovMaxDeg));
        persp->updateMatrices();
    } else if (auto ortho = orthoCamera()) {
        const float half_h = ortho->getHeight() * 0.5f;
        const float half_w = ortho->getWidth() * 0.5f;
        const float hw = std::max(half_w, kEpsilon);
        const float hh = std::max(half_h, kEpsilon);
        const float t_min = std::max(kZoomOrthoHalfMin / hw, kZoomOrthoHalfMin / hh);
        const float t_max = std::min(kZoomOrthoHalfMax / hw, kZoomOrthoHalfMax / hh);
        const float t = vne::math::clamp(factor, t_min, t_max);
        const float new_half_w = half_w * t;
        const float new_half_h = half_h * t;
        ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
        ortho->updateMatrices();
    }
}

// ---------------------------------------------------------------------------
// applySceneScaleZoom
// ---------------------------------------------------------------------------

void CameraBehaviorBase::applySceneScaleZoom(float factor) noexcept {
    if (!camera_ || factor <= 0.0f || !std::isfinite(factor)) {
        return;
    }
    zoom_scale_ = vne::math::clamp(zoom_scale_ * factor, kSceneScaleMin, kSceneScaleMax);
    camera_->setSceneScale(zoom_scale_);
    camera_->updateMatrices();
}

// ---------------------------------------------------------------------------
// applyOrthoZoomToCursor
// ---------------------------------------------------------------------------

void CameraBehaviorBase::applyOrthoZoomToCursor(float factor, float mx, float my) noexcept {
    auto ortho = orthoCamera();
    if (!ortho || viewport().width <= 0.0f || viewport().height <= 0.0f) {
        return;
    }
    const vne::math::Vec2f ndc = mouseWindowToNDC(mx, my, viewport().width, viewport().height, graphicsApi());
    const float ndc_x = ndc.x();
    const float ndc_y = ndc.y();
    const float half_w = ortho->getWidth() * 0.5f;
    const float half_h = ortho->getHeight() * 0.5f;

    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front = target - eye;
    const float front_len = front.length();
    front = (front_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);

    const vne::math::Vec3f up = ortho->getUp().normalized();
    vne::math::Vec3f r = front.cross(up);
    const float r_len = r.length();
    r = (r_len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / r_len);

    const vne::math::Vec3f world_at_cursor = target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
    const float hw = std::max(half_w, kEpsilon);
    const float hh = std::max(half_h, kEpsilon);
    const float t_min = std::max(kZoomOrthoHalfMin / hw, kZoomOrthoHalfMin / hh);
    const float t_max = std::min(kZoomOrthoHalfMax / hw, kZoomOrthoHalfMax / hh);
    const float t = vne::math::clamp(factor, t_min, t_max);
    const float new_half_w = half_w * t;
    const float new_half_h = half_h * t;
    const vne::math::Vec3f new_target = world_at_cursor - r * (ndc_x * new_half_w) - up * (ndc_y * new_half_h);
    const vne::math::Vec3f eye_offset = eye - target;

    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
}

}  // namespace vne::interaction
