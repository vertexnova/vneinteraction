/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_pan_zoom_behavior.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Constants (mirrors ortho_pan_zoom_manipulator.cpp)
// ---------------------------------------------------------------------------
namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.ortho_pan_zoom");
constexpr float kEpsilon = 1e-6f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kPanVelocityThreshold = 1e-4f;
}  // namespace

// ---------------------------------------------------------------------------
// ICameraBehavior: setCamera / onResize
// ---------------------------------------------------------------------------

void OrthoPanZoomBehavior::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraBehaviorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "OrthoPanZoomBehavior: camera detached (null camera)";
    }
}

void OrthoPanZoomBehavior::onResize(float width_px, float height_px) noexcept {
    CameraBehaviorBase::onResize(width_px, height_px);
}

// ---------------------------------------------------------------------------
// Pan
// ---------------------------------------------------------------------------

void OrthoPanZoomBehavior::pan(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        VNE_LOG_WARN << "OrthoPanZoomBehavior: pan called without orthographic camera";
        return;
    }
    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front_vec = target - eye;
    float len = front_vec.length();
    front_vec = (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front_vec / len);

    const vne::math::Vec3f up = ortho->getUp().normalized();
    vne::math::Vec3f r = front_vec.cross(up);  // front × up = right (same as ortho_pan_zoom_manipulator)
    len = r.length();
    r = (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);

    const float wppx = ortho->getWidth() / viewport().width;
    const float wppy = ortho->getHeight() / viewport().height;
    const vne::math::Vec3f delta_world = r * (-delta_x_px * wppx) + up * (-delta_y_px * wppy);

    ortho->setPosition(eye + delta_world);
    ortho->setTarget(target + delta_world);
    ortho->updateMatrices();

    if (delta_time > 0.0) {
        pan_velocity_ = delta_world / static_cast<float>(delta_time);
    }
}

// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void OrthoPanZoomBehavior::applyInertia(double delta_time) noexcept {
    auto ortho = orthoCamera();
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

// ---------------------------------------------------------------------------
// fitToAABB / getWorldUnitsPerPixel / resetState
// ---------------------------------------------------------------------------

void OrthoPanZoomBehavior::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        return;
    }
    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front_vec = target - eye;
    float len = front_vec.length();
    front_vec = (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front_vec / len);

    const vne::math::Vec3f up = ortho->getUp().normalized();
    vne::math::Vec3f r = front_vec.cross(up);
    len = r.length();
    r = (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);

    const vne::math::Vec3f corners[8] = {
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

    const float aspect = viewport().width / viewport().height;
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

float OrthoPanZoomBehavior::getWorldUnitsPerPixel() const noexcept {
    auto ortho = orthoCamera();
    return ortho ? (ortho->getHeight() / viewport().height) : 0.0f;
}

void OrthoPanZoomBehavior::resetState() noexcept {
    panning_ = false;
    pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void OrthoPanZoomBehavior::onUpdate(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!panning_) {
        applyInertia(delta_time);
    }
}

// ---------------------------------------------------------------------------
// onAction
// ---------------------------------------------------------------------------

bool OrthoPanZoomBehavior::onAction(CameraActionType action,
                                    const CameraCommandPayload& payload,
                                    double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return false;
    }
    switch (action) {
        case CameraActionType::eBeginPan:
            panning_ = true;
            pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
            return true;

        case CameraActionType::ePanDelta:
            // Accept pan delta even when !panning_ (touch pan has no button)
            pan(payload.delta_x_px, payload.delta_y_px, delta_time);
            return true;

        case CameraActionType::eEndPan:
            panning_ = false;
            return true;

        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f && payload.zoom_factor != 1.0f) {
                dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px);
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
