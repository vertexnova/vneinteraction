/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include <vertexnova/math/core/core.h>
#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne::math;

void FollowManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
}

void FollowManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
}

Vec3f FollowManipulator::getTargetWorld() const noexcept {
    if (target_provider_) return target_provider_();
    return target_world_;
}

void FollowManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    float dt = static_cast<float>(delta_time);
    if (dt <= 0.0f) return;
    Vec3f target = getTargetWorld();
    Vec3f desired_eye = target + offset_world_;
    Vec3f eye = camera_->getPosition();
    float alpha = 1.0f - std::exp(-damping_ * dt);
    Vec3f new_eye = eye + (desired_eye - eye) * alpha;
    camera_->setPosition(new_eye);
    camera_->setTarget(target);
    camera_->setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
}

void FollowManipulator::handleMouseMove(float, float, float, float, double) noexcept {}

void FollowManipulator::handleMouseButton(int, bool, float, float, double) noexcept {}

void FollowManipulator::handleMouseScroll(float, float scroll_y, float, float, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) return;
    offset_world_.z() *= (scroll_y > 0.0f) ? 0.9f : 1.1f;
    if (std::abs(offset_world_.z()) < 0.1f)
        offset_world_.z() = (offset_world_.z() < 0.0f) ? -0.1f : 0.1f;
}

void FollowManipulator::handleKeyboard(int, bool, double) noexcept {}

void FollowManipulator::handleTouchPan(const TouchPan&, double) noexcept {}

void FollowManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (pinch.scale <= 0.0f) return;
    offset_world_.z() *= (1.0f / pinch.scale);
}

}  // namespace vne::interaction
