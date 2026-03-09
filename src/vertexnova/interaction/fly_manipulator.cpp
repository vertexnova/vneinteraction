/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFitToAabbDistanceFactor = 2.5f;
}  // namespace

FlyManipulator::FlyManipulator() noexcept {
    zoom_speed_ = 0.5f;
}

void FlyManipulator::setViewportSize(float width_px, float height_px) noexcept {
    CameraManipulatorBase::setViewportSize(width_px, height_px);
}

vne::math::Vec3f FlyManipulator::upAxis() const noexcept {
    if (!camera_) {
        return vne::math::Vec3f(0.0f, 1.0f, 0.0f);
    }
    vne::math::Vec3f up = camera_->getUp();
    return up.isZero(kEpsilon) ? vne::math::Vec3f(0.0f, 1.0f, 0.0f) : up.normalized();
}

vne::math::Vec3f FlyManipulator::upVector() const noexcept {
    return upAxis();
}

void FlyManipulator::resetState() noexcept {
    w_ = false;
    a_ = false;
    s_ = false;
    d_ = false;
    q_ = false;
    e_ = false;
    sprint_ = false;
    slow_ = false;
    looking_ = false;
}

void FlyManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
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
    camera_->updateMatrices();
}

float FlyManipulator::getWorldUnitsPerPixel() const noexcept {
    return 0.0f;
}

}  // namespace vne::interaction
