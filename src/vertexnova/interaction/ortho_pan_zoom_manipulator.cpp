/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include <vertexnova/math/core/core.h>
#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kMinViewportSize = 1.0f;
constexpr float kZoomFactorMin = 0.01f;
constexpr float kZoomFactorMax = 100.0f;
constexpr float kSceneScaleMin = 1e-4f;
constexpr float kSceneScaleMax = 1e4f;
constexpr float kZoomToCursorHalfMin = 1e-3f;
constexpr float kZoomToCursorHalfMax = 1e6f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kMinOrthoExtent = 1e-3f;
constexpr float kPanVelocityThreshold = 1e-4f;
}  // namespace

void OrthoPanZoomManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
}

void OrthoPanZoomManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(kMinViewportSize, width_px);
    viewport_height_ = std::max(kMinViewportSize, height_px);
}

std::shared_ptr<vne::scene::OrthographicCamera> OrthoPanZoomManipulator::getOrtho() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
}

void OrthoPanZoomManipulator::pan(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    auto ortho = getOrtho();
    if (!ortho) {
        return;
    }

    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front = target - eye;
    float len = front.length();
    front = (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / len);

    const vne::math::Vec3f up = ortho->getUp().normalized();
    vne::math::Vec3f r = up.cross(front);
    len = r.length();
    r = (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);

    const float wppx = ortho->getWidth() / viewport_width_;
    const float wppy = ortho->getHeight() / viewport_height_;
    const vne::math::Vec3f delta_world = r * (-delta_x_px * wppx) + up * (delta_y_px * wppy);

    ortho->setPosition(eye + delta_world);
    ortho->setTarget(target + delta_world);
    ortho->updateMatrices();

    if (delta_time > 0.0) {
        pan_velocity_ = delta_world / static_cast<float>(delta_time);
    }
}

void OrthoPanZoomManipulator::zoomToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    auto ortho = getOrtho();
    if (!ortho) {
        return;
    }

    zoom_factor = vne::math::clamp(zoom_factor, kZoomFactorMin, kZoomFactorMax);
    const float ndc_x = (2.0f * mouse_x_px / viewport_width_) - 1.0f;
    const float ndc_y = 1.0f - (2.0f * mouse_y_px / viewport_height_);
    const float half_w = ortho->getWidth() * 0.5f;
    const float half_h = ortho->getHeight() * 0.5f;

    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front = target - eye;
    float len = front.length();
    front = (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / len);

    const vne::math::Vec3f up = ortho->getUp().normalized();
    vne::math::Vec3f r = up.cross(front);
    len = r.length();
    r = (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);

    const vne::math::Vec3f world_at_cursor = target + r * (ndc_x * half_w) + up * (ndc_y * half_h);
    const float new_half_w = vne::math::clamp(half_w * zoom_factor, kZoomToCursorHalfMin, kZoomToCursorHalfMax);
    const float new_half_h = vne::math::clamp(half_h * zoom_factor, kZoomToCursorHalfMin, kZoomToCursorHalfMax);
    const vne::math::Vec3f new_target = world_at_cursor - r * (ndc_x * new_half_w) - up * (ndc_y * new_half_h);
    const vne::math::Vec3f eye_offset = eye - target;

    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
}

void OrthoPanZoomManipulator::applyZoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    auto ortho = getOrtho();
    if (!ortho) {
        return;
    }

    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_factor, kSceneScaleMin, kSceneScaleMax);
            return;
        case ZoomMethod::eDollyToCoi:
        case ZoomMethod::eChangeFov:
            zoomToCursor(zoom_factor, mouse_x_px, mouse_y_px);
            return;
    }
}

void OrthoPanZoomManipulator::applyInertia(double delta_time) noexcept {
    auto ortho = getOrtho();
    if (!ortho || delta_time <= 0.0) {
        return;
    }
    if (pan_velocity_.length() < kPanVelocityThreshold) {
        pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
        return;
    }

    const float dt = static_cast<float>(delta_time);
    const vne::math::Vec3f delta = pan_velocity_ * dt;
    ortho->setPosition(ortho->getPosition() + delta);
    ortho->setTarget(ortho->getTarget() + delta);
    ortho->updateMatrices();
    pan_velocity_ *= std::exp(-pan_damping_ * dt);
}

void OrthoPanZoomManipulator::resetState() noexcept {
    panning_ = false;
    pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void OrthoPanZoomManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    auto ortho = getOrtho();
    if (!ortho) {
        return;
    }

    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front = target - eye;
    float len = front.length();
    front = (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / len);

    const vne::math::Vec3f up = ortho->getUp().normalized();
    vne::math::Vec3f r = up.cross(front);
    len = r.length();
    r = (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);

    vne::math::Vec3f corners[8] = {
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
        max_u = std::max(max_u, std::abs(d.dot(up)));
    }

    max_r = std::max(max_r * kFitToAabbMargin, kMinOrthoExtent);
    max_u = std::max(max_u * kFitToAabbMargin, kMinOrthoExtent);

    const float aspect = viewport_width_ / viewport_height_;
    if (max_r / max_u < aspect) {
        max_r = max_u * aspect;
    } else {
        max_u = max_r / aspect;
    }

    const vne::math::Vec3f eye_offset = eye - target;
    ortho->setBounds(-max_r, max_r, -max_u, max_u, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(center);
    ortho->setPosition(center + eye_offset);
    ortho->updateMatrices();
}

float OrthoPanZoomManipulator::getWorldUnitsPerPixel() const noexcept {
    auto ortho = getOrtho();
    return ortho ? (ortho->getHeight() / viewport_height_) : 0.0f;
}

void OrthoPanZoomManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!panning_) {
        applyInertia(delta_time);
    }
}

void OrthoPanZoomManipulator::handleMouseMove(float, float, float delta_x, float delta_y, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (panning_) {
        pan(delta_x, delta_y, delta_time);
    }
}

void OrthoPanZoomManipulator::handleMouseButton(int button, bool pressed, float, float, double) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (button == static_cast<int>(MouseButton::eMiddle) || button == static_cast<int>(MouseButton::eRight)) {
        panning_ = pressed;
        if (pressed) {
            pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
        }
    }
}

void OrthoPanZoomManipulator::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    applyZoom((scroll_y > 0.0f) ? (1.0f / zoom_speed_) : zoom_speed_, mouse_x, mouse_y);
}

void OrthoPanZoomManipulator::handleKeyboard(int, bool, double) noexcept {}

void OrthoPanZoomManipulator::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    this->pan(pan.delta_x_px, pan.delta_y_px, delta_time);
}

void OrthoPanZoomManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) {
        return;
    }
    applyZoom(1.0f / pinch.scale, pinch.center_x_px, pinch.center_y_px);
}

}  // namespace vne::interaction
