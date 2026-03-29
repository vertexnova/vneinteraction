/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/follow_behavior.h"
#include "vertexnova/interaction/behavior_utils.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Constants (mirrors follow_manipulator.cpp)
// ---------------------------------------------------------------------------
namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.follow");
constexpr float kOffsetMinLength = 0.1f;
constexpr float kOffsetMaxLength = 1e4f;
constexpr float kEpsilon = 1e-6f;
}  // namespace

// ---------------------------------------------------------------------------
// ICameraBehavior: setCamera / onResize
// ---------------------------------------------------------------------------

void FollowBehavior::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraBehaviorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "FollowBehavior: camera detached (null camera)";
    }
}

void FollowBehavior::onResize(float width_px, float height_px) noexcept {
    CameraBehaviorBase::onResize(width_px, height_px);
}

void FollowBehavior::setOffset(const vne::math::Vec3f& offset) noexcept {
    if (offset.length() > kOffsetMinLength) {
        offset_world_ = offset;
    } else {
        VNE_LOG_WARN << "FollowBehavior: setOffset ignored — offset length " << offset.length() << " is below minimum "
                     << kOffsetMinLength;
    }
}

void FollowBehavior::setWorldUp(const vne::math::Vec3f& up) noexcept {
    if (up.length() > kEpsilon) {
        world_up_ = up.normalized();
    } else {
        VNE_LOG_WARN << "FollowBehavior: setWorldUp called with zero-length vector, ignoring";
    }
}

// ---------------------------------------------------------------------------
// Target
// ---------------------------------------------------------------------------

vne::math::Vec3f FollowBehavior::getTargetWorld() const noexcept {
    return target_provider_ ? target_provider_() : target_world_;
}

// ---------------------------------------------------------------------------
// fitToAABB / getWorldUnitsPerPixel
// ---------------------------------------------------------------------------

void FollowBehavior::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    target_world_ = (min_world + max_world) * 0.5f;
}

float FollowBehavior::getWorldUnitsPerPixel() const noexcept {
    if (auto ortho = orthoCamera()) {
        return ortho->getHeight() / viewport().height;
    }
    if (auto persp = perspCamera()) {
        const float dist = offset_world_.length();
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * dist * vne::math::tan(fov_y_rad * 0.5f) / viewport().height;
    }
    return 1.0f;
}

// ---------------------------------------------------------------------------
// Zoom (applyDolly: offset scaling for eDollyToCoi; eSceneScale / eChangeFov via base dispatchZoom)
// ---------------------------------------------------------------------------

void FollowBehavior::applyDolly(float factor, float /*mx*/, float /*my*/) noexcept {
    if (!camera_ || factor <= 0.0f) {
        return;
    }
    const vne::math::Vec3f new_offset = offset_world_ * factor;
    const float len = new_offset.length();
    if (len > kOffsetMinLength && len < kOffsetMaxLength) {
        offset_world_ = new_offset;
    }
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void FollowBehavior::onUpdate(double delta_time) noexcept {
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

    const vne::math::Vec3f view_dir = (target - new_eye).normalized();
    vne::math::Vec3f up_hint = world_up_;
    if (std::abs(view_dir.dot(world_up_)) > 0.99f) {
        // world_up_ is collinear with view_dir — pick the first non-collinear candidate
        const vne::math::Vec3f candidates[3] = {{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        for (const auto& c : candidates) {
            if (std::abs(view_dir.dot(c)) < 0.99f) {
                up_hint = c;
                break;
            }
        }
    }
    camera_->lookAt(new_eye, target, up_hint);
    camera_->updateMatrices();
}

// ---------------------------------------------------------------------------
// onAction
// ---------------------------------------------------------------------------

bool FollowBehavior::onAction(CameraActionType action,
                              const CameraCommandPayload& payload,
                              double /*delta_time*/) noexcept {
    if (!enabled_ || !camera_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f && payload.zoom_factor != 1.0f) {
                dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px);
                return true;
            }
            VNE_LOG_DEBUG << "FollowBehavior: ignoring zoom with factor=" << payload.zoom_factor;
            return false;

        case CameraActionType::eResetView:
            resetState();
            return true;

        default:
            return false;
    }
}

}  // namespace vne::interaction
