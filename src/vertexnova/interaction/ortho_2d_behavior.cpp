/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_2d_behavior.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>

#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Constants (mirrors ortho_pan_zoom_manipulator.cpp)
// ---------------------------------------------------------------------------
namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.ortho_2d");
constexpr float kEpsilon = 1e-6f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kPanVelocityThreshold = 1e-4f;
constexpr float kPanVelocityBlendRate = 25.0f;  // EMA time-constant reciprocal (1/s)
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
/** Skip in-plane rotation when |angle| is below this (radians); avoids no-op quaternion work. */
constexpr float kMinRotationAngleRad = 1e-8f;

/**
 * Pan inertia is stored in world units/s. Zoom that scales ortho half-extents by s also scales
 * world-units-per-pixel by s, so screen-space speed ~ |v_world| / s. Scale v_world by s to keep
 * on-screen coasting consistent (matches applyOrthoZoomToCursor / applyFovZoom ortho paths).
 * eSceneScale does not change the ortho frustum here — leave velocity unchanged.
 */
void scalePanVelocityWithOrthoExtentChange(vne::math::Vec3f& pan_velocity,
                                          ZoomMethod method,
                                          float zoom_factor) noexcept {
    float extent_scale = 1.0f;
    switch (method) {
        case ZoomMethod::eDollyToCoi:
            extent_scale = zoom_factor;
            break;
        case ZoomMethod::eChangeFov:
            extent_scale = zoom_factor;
            break;
        case ZoomMethod::eSceneScale:
            return;
    }
    pan_velocity *= extent_scale;
}
}  // namespace

// ---------------------------------------------------------------------------
// ICameraBehavior: setCamera / onResize
// ---------------------------------------------------------------------------

void Ortho2DBehavior::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    CameraBehaviorBase::setCamera(std::move(camera));
    if (!camera_) {
        VNE_LOG_DEBUG << "Ortho2DBehavior: camera detached (null camera)";
    }
}

void Ortho2DBehavior::onResize(float width_px, float height_px) noexcept {
    CameraBehaviorBase::onResize(width_px, height_px);
}

// ---------------------------------------------------------------------------
// Pan
// ---------------------------------------------------------------------------

void Ortho2DBehavior::pan(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        VNE_LOG_WARN << "Ortho2DBehavior: pan called without orthographic camera";
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
    const vne::math::Vec3f delta_world = r * (-delta_x_px * wppx) + up * (delta_y_px * wppy);

    ortho->setPosition(eye + delta_world);
    ortho->setTarget(target + delta_world);
    ortho->updateMatrices();

    if (delta_time > 0.0) {
        const vne::math::Vec3f sample = delta_world / static_cast<float>(delta_time);
        const float blend = 1.0f - std::exp(-kPanVelocityBlendRate * static_cast<float>(delta_time));
        pan_velocity_ = pan_velocity_ + (sample - pan_velocity_) * blend;
    }
}

// ---------------------------------------------------------------------------
// In-plane rotation (slice spin about view axis through target)
// ---------------------------------------------------------------------------

void Ortho2DBehavior::rotateInPlane(float delta_x_px, float /*delta_y_px*/) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        VNE_LOG_WARN << "Ortho2DBehavior: rotateInPlane called without orthographic camera";
        return;
    }
    const float angle_rad = delta_x_px * rotation_deg_per_px_ * kDegToRad;
    if (!std::isfinite(angle_rad)) {
        VNE_LOG_ERROR << "Ortho2DBehavior: rotateInPlane non-finite angle (delta_x_px=" << delta_x_px
                      << ", rotation_deg_per_px=" << rotation_deg_per_px_ << ")";
        return;
    }
    if (std::abs(angle_rad) < kMinRotationAngleRad) {
        return;
    }

    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f axis = target - eye;
    const float axis_len = axis.length();
    if (axis_len < kEpsilon) {
        VNE_LOG_WARN << "Ortho2DBehavior: rotateInPlane degenerate view axis (eye coincident with target)";
        return;
    }
    axis = axis / axis_len;

    vne::math::Vec3f offset = eye - target;
    vne::math::Vec3f up = ortho->getUp().normalized();
    const vne::math::Quatf q = vne::math::Quatf::fromAxisAngle(axis, angle_rad);
    offset = q.rotate(offset);
    up = q.rotate(up).normalized();

    const vne::math::Vec3f new_position = target + offset;
    ortho->lookAt(new_position, target, up);
    ortho->updateMatrices();
}

// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void Ortho2DBehavior::applyInertia(double delta_time) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        VNE_LOG_DEBUG << "Ortho2DBehavior: applyInertia skipped (no orthographic camera)";
        return;
    }
    if (delta_time <= 0.0) {
        return;
    }
    if (!std::isfinite(static_cast<double>(delta_time))) {
        VNE_LOG_ERROR << "Ortho2DBehavior: applyInertia non-finite delta_time";
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

void Ortho2DBehavior::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        VNE_LOG_WARN << "Ortho2DBehavior: fitToAABB called without orthographic camera";
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

float Ortho2DBehavior::getWorldUnitsPerPixel() const noexcept {
    auto ortho = orthoCamera();
    return ortho ? (ortho->getHeight() / viewport().height) : 0.0f;
}

void Ortho2DBehavior::resetState() noexcept {
    panning_ = false;
    rotating_ = false;
    pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// onUpdate
// ---------------------------------------------------------------------------

void Ortho2DBehavior::onUpdate(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!panning_ && !rotating_) {
        applyInertia(delta_time);
    }
}

// ---------------------------------------------------------------------------
// onAction
// ---------------------------------------------------------------------------

bool Ortho2DBehavior::onAction(CameraActionType action,
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

        case CameraActionType::eBeginRotate:
            rotating_ = true;
            pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
            return true;

        case CameraActionType::eRotateDelta:
            rotateInPlane(payload.delta_x_px, payload.delta_y_px);
            return true;

        case CameraActionType::eEndRotate:
            rotating_ = false;
            return true;

        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f && payload.zoom_factor != 1.0f) {
                scalePanVelocityWithOrthoExtentChange(pan_velocity_, getZoomMethod(), payload.zoom_factor);
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
