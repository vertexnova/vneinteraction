/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/track_behavior.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Constants (mirrors follow_manipulator.cpp)
// ---------------------------------------------------------------------------
namespace {
constexpr float kFovMinDeg = 5.0f;
constexpr float kFovMaxDeg = 120.0f;
constexpr float kSceneScaleMin = 1e-4f;
constexpr float kSceneScaleMax = 1e4f;
constexpr float kOffsetMinLength = 0.1f;
constexpr float kOffsetMaxLength = 1e4f;
}  // namespace

// ---------------------------------------------------------------------------
// Camera helpers
// ---------------------------------------------------------------------------

std::shared_ptr<vne::scene::PerspectiveCamera> TrackBehavior::perspCamera() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_);
}

std::shared_ptr<vne::scene::OrthographicCamera> TrackBehavior::orthoCamera() const noexcept {
    return std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
}

// ---------------------------------------------------------------------------
// ICameraBehavior: setCamera / setViewportSize
// ---------------------------------------------------------------------------

void TrackBehavior::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
}

void TrackBehavior::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
}

// ---------------------------------------------------------------------------
// Target
// ---------------------------------------------------------------------------

vne::math::Vec3f TrackBehavior::getTargetWorld() const noexcept {
    return target_provider_ ? target_provider_() : target_world_;
}

// ---------------------------------------------------------------------------
// fitToAABB / getWorldUnitsPerPixel
// ---------------------------------------------------------------------------

void TrackBehavior::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    target_world_ = (min_world + max_world) * 0.5f;
}

float TrackBehavior::getWorldUnitsPerPixel() const noexcept {
    if (auto ortho = orthoCamera()) {
        return ortho->getHeight() / viewport_height_;
    }
    if (auto persp = perspCamera()) {
        const float dist = offset_world_.length();
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * dist * vne::math::tan(fov_y_rad * 0.5f) / viewport_height_;
    }
    return 1.0f;
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------

void TrackBehavior::applyZoom(float zoom_factor) noexcept {
    if (!camera_) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_factor, kSceneScaleMin, kSceneScaleMax);
            return;
        case ZoomMethod::eDollyToCoi: {
            const vne::math::Vec3f new_offset = offset_world_ * zoom_factor;
            const float len = new_offset.length();
            if (len > kOffsetMinLength && len < kOffsetMaxLength) {
                offset_world_ = new_offset;
            }
            return;
        }
        case ZoomMethod::eChangeFov:
            if (auto persp = perspCamera()) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(vne::math::clamp(fov * zoom_factor, kFovMinDeg, kFovMaxDeg));
                persp->updateMatrices();
            } else {
                // No perspective camera: fall back to dolly offset
                const vne::math::Vec3f new_offset = offset_world_ * zoom_factor;
                const float len = new_offset.length();
                if (len > kOffsetMinLength && len < kOffsetMaxLength) {
                    offset_world_ = new_offset;
                }
            }
            return;
    }
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void TrackBehavior::update(double delta_time) noexcept {
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

// ---------------------------------------------------------------------------
// onAction
// ---------------------------------------------------------------------------

bool TrackBehavior::onAction(CameraActionType action,
                             const CameraCommandPayload& payload,
                             double /*delta_time*/) noexcept {
    if (!enabled_ || !camera_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f) {
                applyZoom(payload.zoom_factor);
                return true;
            }
            return false;

        case CameraActionType::eResetView:
            resetState();
            return true;

        default:
            return false;
    }
}

}  // namespace vne::interaction
