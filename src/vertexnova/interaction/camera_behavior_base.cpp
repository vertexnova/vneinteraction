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
            onZoomDolly(factor, mx, my);
            return;
    }
}

void CameraBehaviorBase::onZoomDolly(float factor, float mx, float my) noexcept {
    // Default: handle ortho cursor-anchored zoom; persp no-op (behaviors override).
    if (orthoCamera()) {
        applyOrthoZoomToCursor(factor, mx, my);
    }
}

// ---------------------------------------------------------------------------
// applyFovZoom
// ---------------------------------------------------------------------------

void CameraBehaviorBase::applyFovZoom(float factor) noexcept {
    if (!camera_) {
        return;
    }
    const float mult = (factor < 1.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_;

    if (auto persp = perspCamera()) {
        persp->setFieldOfView(vne::math::clamp(persp->getFieldOfView() * mult, kFovMinDeg, kFovMaxDeg));
        persp->updateMatrices();
        return;
    }

    if (auto ortho = orthoCamera()) {
        const float half_h = ortho->getHeight() * 0.5f;
        const float half_w = ortho->getWidth() * 0.5f;
        const float aspect = (half_h > kEpsilon) ? (half_w / half_h) : 1.0f;
        const float new_half_h = std::max(kMinOrthoExtent, half_h * mult);
        const float new_half_w = std::max(kMinOrthoExtent, new_half_h * aspect);
        ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
        ortho->updateMatrices();
    }
}

// ---------------------------------------------------------------------------
// applySceneScaleZoom
// ---------------------------------------------------------------------------

void CameraBehaviorBase::applySceneScaleZoom(float factor) noexcept {
    if (!camera_) {
        return;
    }
    zoom_scale_ = vne::math::clamp(zoom_scale_ * factor, kSceneScaleMin, kSceneScaleMax);
    // ICamera no longer exposes scene scale; zoom_scale_ is tracked for getZoomScale() and app use.
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
    const vne::math::Vec2f ndc = mouseToNDC(mx, my, viewport().width, viewport().height);
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
    const float new_half_w = vne::math::clamp(half_w * factor, kZoomOrthoHalfMin, kZoomOrthoHalfMax);
    const float new_half_h = vne::math::clamp(half_h * factor, kZoomOrthoHalfMin, kZoomOrthoHalfMax);
    const vne::math::Vec3f new_target = world_at_cursor - r * (ndc_x * new_half_w) - up * (ndc_y * new_half_h);
    const vne::math::Vec3f eye_offset = eye - target;

    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
}

}  // namespace vne::interaction
