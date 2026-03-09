/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <algorithm>
#include <cmath>

namespace vne::interaction {

vne::math::Vec3f FollowManipulator::getTargetWorld() const noexcept {
    return target_provider_ ? target_provider_() : target_world_;
}

void FollowManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
}

void FollowManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
}

void FollowManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    target_world_ = (min_world + max_world) * 0.5f;
}

void FollowManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }

    const float dt = static_cast<float>(delta_time);
    if (dt <= 0.0f) {
        return;
    }

    const vne::math::Vec3f target = getTargetWorld();
    const vne::math::Vec3f desired_eye = target + offset_world_;
    const vne::math::Vec3f eye = camera_->getPosition();
    const float alpha = 1.0f - std::exp(-damping_ * dt);
    const vne::math::Vec3f new_eye = eye + (desired_eye - eye) * alpha;

    camera_->setPosition(new_eye);
    camera_->setTarget(target);
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
}

void FollowManipulator::handleMouseMove(float, float, float, float, double) noexcept {}
void FollowManipulator::handleMouseButton(int, bool, float, float, double) noexcept {}
void FollowManipulator::handleKeyboard(int, bool, double) noexcept {}
void FollowManipulator::handleTouchPan(const TouchPan&, double) noexcept {}

void FollowManipulator::applyZoom(float zoom_factor) noexcept {
    if (!camera_) {
        return;
    }

    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_factor, 1e-4f, 1e4f);
            return;
        case ZoomMethod::eDollyToCoi: {
            const vne::math::Vec3f new_offset = offset_world_ * zoom_factor;
            const float len = new_offset.length();
            if (len > 0.1f && len < 1e4f) {
                offset_world_ = new_offset;
            }
            return;
        }
        case ZoomMethod::eChangeFov:
            if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(vne::math::clamp(fov * zoom_factor, 5.0f, 120.0f));
                persp->updateMatrices();
            } else {
                const vne::math::Vec3f new_offset = offset_world_ * zoom_factor;
                const float len = new_offset.length();
                if (len > 0.1f && len < 1e4f) {
                    offset_world_ = new_offset;
                }
            }
            return;
    }
}

void FollowManipulator::handleMouseScroll(float, float scroll_y, float, float, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    applyZoom((scroll_y > 0.0f) ? (1.0f / zoom_speed_) : zoom_speed_);
}

void FollowManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || pinch.scale <= 0.0f) {
        return;
    }
    applyZoom(1.0f / pinch.scale);
}

float FollowManipulator::getWorldUnitsPerPixel() const noexcept {
    return 0.0f;
}

}  // namespace vne::interaction
