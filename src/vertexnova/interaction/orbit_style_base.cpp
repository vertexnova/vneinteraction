/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/detail/orbit_style_base.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kFrontZNearVertical = 0.999f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kMaxOrbitDistance = 1e6f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kFovMinDeg = 5.0f;
constexpr float kFovMaxDeg = 120.0f;
constexpr float kSceneScaleMin = 1e-4f;
constexpr float kSceneScaleMax = 1e4f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kMinOrthoExtent = 1e-3f;
constexpr float kInertiaPanThreshold = 1e-4f;
constexpr int kKeyLeftShift = 340;
constexpr int kKeyRightShift = 344;
constexpr double kMinDeltaTimeForInertia = 0.001;  // 1ms: ignore tiny-dt inertia samples
constexpr float kPanVelocityBlend = 0.35f;         // EMA blend factor for pan inertia velocity
constexpr float kZoomToCursorStrength = 0.3f;      // how aggressively COI migrates toward cursor on zoom
constexpr float kFitAnimationSpeed = 10.0f;        // exponential approach rate for fitToAABB animation
constexpr float kFitConvergeThreshold = 1e-3f;     // stop animating when this close to target
}  // namespace

bool OrbitStyleBase::isPerspective() const noexcept {
    return static_cast<bool>(perspCamera());
}

bool OrbitStyleBase::isOrthographic() const noexcept {
    return static_cast<bool>(orthoCamera());
}

vne::math::Vec3f OrbitStyleBase::computeRight(const vne::math::Vec3f& front) const noexcept {
    vne::math::Vec3f r = world_up_.cross(front);
    float len = r.length();
    if (len < kEpsilon) {
        const vne::math::Vec3f fallback_up = (std::abs(front.z()) < kFrontZNearVertical)
                                                 ? vne::math::Vec3f(0.0f, 0.0f, 1.0f)
                                                 : vne::math::Vec3f(0.0f, 1.0f, 0.0f);
        r = fallback_up.cross(front);
        len = r.length();
    }
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

vne::math::Vec3f OrbitStyleBase::computeUp(const vne::math::Vec3f& front,
                                           const vne::math::Vec3f& right) const noexcept {
    vne::math::Vec3f up = front.cross(right);
    const float len = up.length();
    return (len < kEpsilon) ? world_up_ : (up / len);
}

void OrbitStyleBase::beginPan(float x_px, float y_px) noexcept {
    interaction_.panning = true;
    interaction_.last_x_px = x_px;
    interaction_.last_y_px = y_px;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitStyleBase::dragPan(float, float, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    vne::math::Vec3f delta_world(0.0f, 0.0f, 0.0f);
    if (isPerspective()) {
        const float wpp = getWorldUnitsPerPixel();
        delta_world = r * (-delta_x_px * wpp * pan_speed_) + u * (delta_y_px * wpp * pan_speed_);
    } else {
        auto ortho = orthoCamera();
        if (ortho) {
            const float wppx = ortho->getWidth() / viewport_width_;
            const float wppy = ortho->getHeight() / viewport_height_;
            delta_world = r * (-delta_x_px * wppx * pan_speed_) + u * (delta_y_px * wppy * pan_speed_);
        }
    }

    if (pivot_mode_ == RotationPivotMode::eFixedWorld) {
        // Translate eye+target together; coi_world_ stays pinned at its original position
        camera_->setPosition(camera_->getPosition() + delta_world);
        camera_->setTarget(camera_->getTarget() + delta_world);
        camera_->updateMatrices();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    } else {
        // eCoi / eViewCenter: move the center of interest
        coi_world_ += delta_world;
        applyToCamera();
    }

    if (delta_time >= kMinDeltaTimeForInertia) {
        const vne::math::Vec3f sample = delta_world / static_cast<float>(delta_time);
        inertia_pan_velocity_ = inertia_pan_velocity_ + (sample - inertia_pan_velocity_) * kPanVelocityBlend;
    }
}

void OrbitStyleBase::endPan(double) noexcept {
    interaction_.panning = false;
    if (pivot_mode_ == RotationPivotMode::eViewCenter && camera_) {
        // Update COI to wherever the camera is now looking — rotate around new view center
        coi_world_ = camera_->getTarget();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
        onPivotChanged();
    }
}

void OrbitStyleBase::zoomOrthoToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    auto ortho = orthoCamera();
    if (!ortho) {
        return;
    }
    const float ndc_x = (2.0f * mouse_x_px / viewport_width_) - 1.0f;
    const float ndc_y = 1.0f - (2.0f * mouse_y_px / viewport_height_);
    const float half_w = ortho->getWidth() * 0.5f;
    const float half_h = ortho->getHeight() * 0.5f;
    const vne::math::Vec3f eye = ortho->getPosition();
    const vne::math::Vec3f target = ortho->getTarget();
    vne::math::Vec3f front = target - eye;
    const float front_len = front.length();
    front = (front_len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);
    const vne::math::Vec3f r = computeRight(front);
    const vne::math::Vec3f u = computeUp(front, r);
    const vne::math::Vec3f world_at_cursor = target + r * (ndc_x * half_w) + u * (ndc_y * half_h);
    const float new_half_w = half_w * zoom_factor;
    const float new_half_h = half_h * zoom_factor;
    const vne::math::Vec3f new_target = world_at_cursor - r * (ndc_x * new_half_w) - u * (ndc_y * new_half_h);
    const vne::math::Vec3f eye_offset = eye - target;
    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
    coi_world_ = new_target;
}

void OrbitStyleBase::zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    if (!camera_) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_factor, kSceneScaleMin, kSceneScaleMax);
            return;
        case ZoomMethod::eChangeFov: {
            auto persp = perspCamera();
            if (persp) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(
                    vne::math::clamp(fov * ((zoom_factor < 1.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_),
                                     kFovMinDeg,
                                     kFovMaxDeg));
                persp->updateMatrices();
                return;
            }
            [[fallthrough]];
        }
        case ZoomMethod::eDollyToCoi:
            if (orthoCamera()) {
                zoomOrthoToCursor(zoom_factor, mouse_x_px, mouse_y_px);
                return;
            }
            {
                // Perspective dolly with cursor-tracking COI shift (Blender-style zoom-to-cursor)
                const float old_dist = orbit_distance_;
                orbit_distance_ = vne::math::clamp(orbit_distance_ * zoom_factor, kMinOrbitDistance, kMaxOrbitDistance);

                // Compute world position under cursor on the focal plane
                const vne::math::Vec3f front = computeFront();
                const vne::math::Vec3f r = computeRight(front);
                const vne::math::Vec3f u = computeUp(front, r);
                auto persp = perspCamera();
                if (persp && viewport_width_ > 0.0f && viewport_height_ > 0.0f) {
                    const float ndc_x = (2.0f * mouse_x_px / viewport_width_) - 1.0f;
                    const float ndc_y = 1.0f - (2.0f * mouse_y_px / viewport_height_);
                    const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
                    const float half_h = old_dist * vne::math::tan(fov_y_rad * 0.5f);
                    const float half_w = half_h * (viewport_width_ / viewport_height_);
                    const vne::math::Vec3f cursor_world =
                        camera_->getPosition() + front * old_dist + r * (ndc_x * half_w) + u * (ndc_y * half_h);
                    const vne::math::Vec3f to_cursor = cursor_world - coi_world_;
                    // Shift COI toward cursor proportionally to zoom step; guard against large jumps
                    const float shift_t = (1.0f - zoom_factor) * kZoomToCursorStrength;
                    if (to_cursor.length() < old_dist * 2.0f) {
                        coi_world_ += to_cursor * shift_t;
                    }
                }
                applyToCamera();
                // If eViewCenter, resync subclass rotation state to new COI
                if (pivot_mode_ == RotationPivotMode::eViewCenter) {
                    onPivotChanged();
                }
            }
            return;
    }
}

void OrbitStyleBase::doPanInertia(double delta_time) noexcept {
    if (delta_time <= 0.0 || !camera_) {
        return;
    }
    if (inertia_pan_velocity_.length() <= kInertiaPanThreshold) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    const vne::math::Vec3f delta = inertia_pan_velocity_ * dt;
    inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);

    if (pivot_mode_ == RotationPivotMode::eFixedWorld) {
        // Translate eye+target; leave coi_world_ pinned
        camera_->setPosition(camera_->getPosition() + delta);
        camera_->setTarget(camera_->getTarget() + delta);
        camera_->updateMatrices();
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), kMinOrbitDistance);
    } else {
        coi_world_ += delta;
        applyToCamera();
    }
}

void OrbitStyleBase::applyInertia(double delta_time) noexcept {
    doPanInertia(delta_time);
}

void OrbitStyleBase::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    // Smooth fitToAABB animation — exponential approach toward target values
    if (animating_fit_ && delta_time > 0.0) {
        const float alpha = 1.0f - std::exp(-kFitAnimationSpeed * static_cast<float>(delta_time));
        orbit_distance_ += (target_orbit_distance_ - orbit_distance_) * alpha;
        coi_world_ = coi_world_ + (target_coi_world_ - coi_world_) * alpha;
        applyToCamera();
        const float dist_diff = std::abs(orbit_distance_ - target_orbit_distance_);
        const float coi_diff = (coi_world_ - target_coi_world_).length();
        if (dist_diff < kFitConvergeThreshold && coi_diff < kFitConvergeThreshold) {
            orbit_distance_ = target_orbit_distance_;
            coi_world_ = target_coi_world_;
            applyToCamera();
            animating_fit_ = false;
            onPivotChanged();
        }
    }
    if (!animating_fit_ && !interaction_.rotating && !interaction_.panning) {
        applyInertia(delta_time);
    }
}

void OrbitStyleBase::applyCommand(CameraActionType action,
                                  const CameraCommandPayload& payload,
                                  double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    switch (action) {
        case CameraActionType::eBeginRotate:
            beginRotate(payload.x_px, payload.y_px);
            break;
        case CameraActionType::eRotateDelta:
            if (interaction_.rotating) {
                dragRotate(payload.delta_x_px, payload.delta_y_px, delta_time);
            }
            break;
        case CameraActionType::eEndRotate:
            endRotate(delta_time);
            break;
        case CameraActionType::eBeginPan:
            beginPan(payload.x_px, payload.y_px);
            break;
        case CameraActionType::ePanDelta:
            if (interaction_.panning) {
                dragPan(payload.x_px, payload.y_px, payload.delta_x_px, payload.delta_y_px, delta_time);
                interaction_.last_x_px = payload.x_px;
                interaction_.last_y_px = payload.y_px;
            }
            break;
        case CameraActionType::eEndPan:
            endPan(delta_time);
            break;
        case CameraActionType::eZoomAtCursor:
            if (payload.zoom_factor > 0.0f) {
                zoom(payload.zoom_factor, payload.x_px, payload.y_px);
            }
            break;
        case CameraActionType::eOrbitPanModifier:
            interaction_.modifier_shift = payload.pressed;
            break;
        default:
            break;
    }
}

void OrbitStyleBase::handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (interaction_.rotating) {
        dragRotate(delta_x, delta_y, delta_time);
    } else if (interaction_.panning) {
        dragPan(x, y, delta_x, delta_y, delta_time);
    }
    interaction_.last_x_px = x;
    interaction_.last_y_px = y;
}

void OrbitStyleBase::handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    const bool pan_alias = (button == static_cast<int>(MouseButton::eMiddle));
    if (pressed) {
        if (button == button_map_.rotate && !interaction_.modifier_shift) {
            beginRotate(x, y);
        } else if (button == button_map_.pan || pan_alias
                   || (button == button_map_.rotate && interaction_.modifier_shift)) {
            beginPan(x, y);
        }
    } else {
        if (button == button_map_.rotate) {
            endRotate(delta_time);
        }
        if (button == button_map_.pan || pan_alias || button == button_map_.rotate) {
            endPan(delta_time);
        }
    }
}

void OrbitStyleBase::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    zoom((scroll_y > 0.0f) ? (1.0f / zoom_speed_) : zoom_speed_, mouse_x, mouse_y);
}

void OrbitStyleBase::handleKeyboard(int key, bool pressed, double) noexcept {
    if (key == kKeyLeftShift || key == kKeyRightShift) {
        interaction_.modifier_shift = pressed;
    }
}

void OrbitStyleBase::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    dragRotate(pan.delta_x_px, pan.delta_y_px, delta_time);
}

void OrbitStyleBase::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) {
        return;
    }
    zoom(1.0f / pinch.scale, pinch.center_x_px, pinch.center_y_px);
}

void OrbitStyleBase::resetState() noexcept {
    interaction_.rotating = false;
    interaction_.panning = false;
    interaction_.modifier_shift = false;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitStyleBase::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    const vne::math::Vec3f extents = max_world - min_world;
    float radius = extents.length() * 0.5f;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }
    // Compute targets. Apply them immediately so getters are correct on the same frame,
    // then animate toward them smoothly in update(). The camera is placed at the target
    // state right away; update() will drive any remaining delta on subsequent frames
    // (zero delta if called right after fitToAABB, harmless).
    target_coi_world_ = center;
    coi_world_ = center;
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float aspect = std::max(viewport_width_ / viewport_height_, kMinOrthoExtent);
        const float fov_x_rad = 2.0f * vne::math::atan(vne::math::tan(fov_y_rad * 0.5f) * aspect);
        const float dist_y = radius / vne::math::tan(fov_y_rad * 0.5f);
        const float dist_x = radius / vne::math::tan(fov_x_rad * 0.5f);
        target_orbit_distance_ = std::max(dist_x, dist_y) * kFitToAabbMargin;
        orbit_distance_ = target_orbit_distance_;
        applyToCamera();
        onPivotChanged();
        animating_fit_ = true;
    } else if (auto ortho = orthoCamera()) {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f r = computeRight(front);
        const vne::math::Vec3f u = computeUp(front, r);
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
            max_u = std::max(max_u, std::abs(d.dot(u)));
        }
        max_r = std::max(max_r * kFitToAabbMargin, kMinOrthoExtent);
        max_u = std::max(max_u * kFitToAabbMargin, kMinOrthoExtent);
        const float aspect = viewport_width_ / viewport_height_;
        if (max_r / max_u < aspect) {
            max_r = max_u * aspect;
        } else {
            max_u = max_r / aspect;
        }
        ortho->setBounds(-max_r, max_r, -max_u, max_u, ortho->getNearPlane(), ortho->getFarPlane());
        // Ortho: apply immediately (bounds change can't be smoothly interpolated)
        coi_world_ = center;
        target_coi_world_ = center;
        applyToCamera();
        onPivotChanged();
    }
}

float OrbitStyleBase::getWorldUnitsPerPixel() const noexcept {
    if (auto ortho = orthoCamera()) {
        return ortho->getHeight() / viewport_height_;
    }
    if (auto persp = perspCamera()) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * orbit_distance_ * vne::math::tan(fov_y_rad * 0.5f) / viewport_height_;
    }
    return 0.0f;
}

}  // namespace vne::interaction
