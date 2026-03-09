/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/detail/camera_manipulator_base.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <algorithm>

namespace vne::interaction {

namespace {
constexpr float kMinViewportSize = 1.0f;
}  // namespace

void CameraManipulatorBase::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(kMinViewportSize, width_px);
    viewport_height_ = std::max(kMinViewportSize, height_px);
}

std::shared_ptr<vne::scene::PerspectiveCamera> CameraManipulatorBase::perspCamera() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_);
}

std::shared_ptr<vne::scene::OrthographicCamera> CameraManipulatorBase::orthoCamera() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
}

}  // namespace vne::interaction
