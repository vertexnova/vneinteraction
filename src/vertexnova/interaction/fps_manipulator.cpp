/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbDistanceFactor = 2.5f;
}  // namespace

FpsManipulator::FpsManipulator() noexcept {
    zoom_speed_ = 0.5f;
}

void FpsManipulator::setViewportSize(float width_px, float height_px) noexcept {
    CameraManipulatorBase::setViewportSize(width_px, height_px);
}

void FpsManipulator::setWorldUp(const vne::math::Vec3f& up) noexcept {
    if (up.length() > kEpsilon) {
        world_up_ = up.normalized();
    }
}

vne::math::Vec3f FpsManipulator::upVector() const noexcept {
    return world_up_;
}

void FpsManipulator::resetState() noexcept {
    input_state_ = FreeLookInputState{};
}

void FpsManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    float radius = (max_world - min_world).length() * 0.5f;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }
    const vne::math::Vec3f f = front();
    camera_->setPosition(center - f * (radius * kFitToAabbDistanceFactor));
    camera_->setTarget(center);
    camera_->setUp(world_up_);
    camera_->updateMatrices();
}

float FpsManipulator::getWorldUnitsPerPixel() const noexcept {
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * vne::math::tan(fov_y_rad * 0.5f) / viewport_height_;
    }
    return 1.0f;
}

}  // namespace vne::interaction
