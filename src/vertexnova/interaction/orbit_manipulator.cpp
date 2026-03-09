/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/core/math_utils.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

namespace {
constexpr float kEpsilon = 1e-6f;
constexpr float kMinViewportSize = 1.0f;
constexpr float kMinOrbitDistance = 0.01f;
constexpr float kMaxOrbitDistance = 1e6f;
constexpr float kDefaultOrbitDistance = 5.0f;
constexpr float kMinRadiusFallback = 1.0f;
constexpr float kPitchMinDeg = -89.0f;
constexpr float kPitchMaxDeg = 89.0f;
constexpr float kFovMinDeg = 5.0f;
constexpr float kFovMaxDeg = 120.0f;
constexpr float kSceneScaleMin = 1e-4f;
constexpr float kSceneScaleMax = 1e4f;
constexpr float kFitToAabbMargin = 1.1f;
constexpr float kMinOrthoExtent = 1e-3f;
constexpr float kInertiaRotThreshold = 1e-3f;
constexpr float kInertiaPanThreshold = 1e-4f;
constexpr int kKeyLeftShift = 340;
constexpr int kKeyRightShift = 344;
}  // namespace

OrbitManipulator::OrbitManipulator() noexcept
    : world_up_(0.0f, 1.0f, 0.0f)
    , coi_world_(0.0f, 0.0f, 0.0f)
    , inertia_pan_velocity_(0.0f, 0.0f, 0.0f) {}

void OrbitManipulator::setWorldUp(const vne::math::Vec3f& world_up) noexcept {
    if (world_up.length() > kEpsilon) {
        world_up_ = world_up.normalized();
    }
}

void OrbitManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncFromCamera();
}

void OrbitManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(kMinViewportSize, width_px);
    viewport_height_ = std::max(kMinViewportSize, height_px);
}

bool OrbitManipulator::isPerspective() const noexcept {
    return static_cast<bool>(std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_));
}

bool OrbitManipulator::isOrthographic() const noexcept {
    return static_cast<bool>(std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_));
}

vne::math::Vec3f OrbitManipulator::computeFront() const noexcept {
    const float yaw_rad = vne::math::degToRad(yaw_deg_);
    const float pitch_rad = vne::math::degToRad(pitch_deg_);
    const float cp = vne::math::cos(pitch_rad);
    vne::math::Vec3f front(vne::math::sin(yaw_rad) * cp, vne::math::sin(pitch_rad), -vne::math::cos(yaw_rad) * cp);
    const float len = front.length();
    return (len < kEpsilon) ? vne::math::Vec3f(0.0f, 0.0f, -1.0f) : (front / len);
}

vne::math::Vec3f OrbitManipulator::computeRight(const vne::math::Vec3f& front) const noexcept {
    vne::math::Vec3f right = world_up_.cross(front);
    const float len = right.length();
    return (len < kEpsilon) ? vne::math::Vec3f(1.0f, 0.0f, 0.0f) : (right / len);
}

vne::math::Vec3f OrbitManipulator::computeUp(const vne::math::Vec3f& front,
                                             const vne::math::Vec3f& right) const noexcept {
    vne::math::Vec3f up = front.cross(right);
    const float len = up.length();
    return (len < kEpsilon) ? world_up_ : (up / len);
}

void OrbitManipulator::syncFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f eye = camera_->getPosition();
    coi_world_ = camera_->getTarget();
    vne::math::Vec3f front = coi_world_ - eye;
    orbit_distance_ = front.length();
    if (orbit_distance_ < kEpsilon) {
        orbit_distance_ = kDefaultOrbitDistance;
        yaw_deg_ = 0.0f;
        pitch_deg_ = 0.0f;
        return;
    }
    front /= orbit_distance_;
    pitch_deg_ = vne::math::radToDeg(vne::math::asin(vne::math::clamp(front.y(), -1.0f, 1.0f)));
    yaw_deg_ = vne::math::radToDeg(vne::math::atan2(front.x(), -front.z()));
}

void OrbitManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f right = computeRight(front);
    const vne::math::Vec3f up = computeUp(front, right);
    const vne::math::Vec3f eye = coi_world_ - front * orbit_distance_;
    camera_->setPosition(eye);
    camera_->setTarget(coi_world_);
    camera_->setUp(up);
    camera_->updateMatrices();
}

void OrbitManipulator::setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept {
    if (space == CenterOfInterestSpace::eWorldSpace || !camera_) {
        coi_world_ = coi;
    } else {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f right = computeRight(front);
        const vne::math::Vec3f up = computeUp(front, right);
        const vne::math::Vec3f eye = camera_->getPosition();
        coi_world_ = eye + right * coi.x() + up * coi.y() + front * coi.z();
    }
    applyToCamera();
}

void OrbitManipulator::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = std::max(distance, kMinOrbitDistance);
    applyToCamera();
}

void OrbitManipulator::setViewDirection(ViewDirection dir) noexcept {
    switch (dir) {
        case ViewDirection::eFront:
            yaw_deg_ = 0.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eBack:
            yaw_deg_ = 180.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eLeft:
            yaw_deg_ = -90.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eRight:
            yaw_deg_ = 90.0f;
            pitch_deg_ = 0.0f;
            break;
        case ViewDirection::eTop:
            yaw_deg_ = 0.0f;
            pitch_deg_ = 89.0f;
            break;
        case ViewDirection::eBottom:
            yaw_deg_ = 0.0f;
            pitch_deg_ = -89.0f;
            break;
        case ViewDirection::eIso:
            yaw_deg_ = 45.0f;
            pitch_deg_ = 35.2643897f;
            break;
    }
    applyToCamera();
}

void OrbitManipulator::beginRotate(float x_px, float y_px) noexcept {
    rotating_ = true;
    last_x_ = x_px;
    last_y_ = y_px;
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
}

void OrbitManipulator::dragRotate(float delta_x_px, float delta_y_px, double delta_time) noexcept {
    yaw_deg_ += delta_x_px * rotation_speed_;
    pitch_deg_ -= delta_y_px * rotation_speed_;
    pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
    applyToCamera();
    if (delta_time > 0.0) {
        const float inv_dt = 1.0f / static_cast<float>(delta_time);
        inertia_rot_speed_x_ = delta_x_px * rotation_speed_ * inv_dt;
        inertia_rot_speed_y_ = -delta_y_px * rotation_speed_ * inv_dt;
    }
}

void OrbitManipulator::endRotate(double) noexcept {
    rotating_ = false;
}

void OrbitManipulator::beginPan(float x_px, float y_px) noexcept {
    panning_ = true;
    last_x_ = x_px;
    last_y_ = y_px;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitManipulator::dragPan(float, float, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f front = computeFront();
    const vne::math::Vec3f right = computeRight(front);
    const vne::math::Vec3f up = computeUp(front, right);
    vne::math::Vec3f delta_world(0.0f, 0.0f, 0.0f);
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        (void)persp;
        const float wpp = getWorldUnitsPerPixel();
        delta_world = right * (-delta_x_px * wpp * pan_speed_) + up * (delta_y_px * wpp * pan_speed_);
    } else if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        const float wppx = ortho->getWidth() / viewport_width_;
        const float wppy = ortho->getHeight() / viewport_height_;
        delta_world = right * (-delta_x_px * wppx * pan_speed_) + up * (delta_y_px * wppy * pan_speed_);
    }
    coi_world_ += delta_world;
    applyToCamera();
    if (delta_time > 0.0) {
        inertia_pan_velocity_ = delta_world / static_cast<float>(delta_time);
    }
}

void OrbitManipulator::endPan(double) noexcept {
    panning_ = false;
}

void OrbitManipulator::applyInertia(double delta_time) noexcept {
    if (delta_time <= 0.0 || !camera_) {
        return;
    }
    const float dt = static_cast<float>(delta_time);
    bool changed = false;
    if (std::abs(inertia_rot_speed_x_) > kInertiaRotThreshold
        || std::abs(inertia_rot_speed_y_) > kInertiaRotThreshold) {
        yaw_deg_ += inertia_rot_speed_x_ * dt;
        pitch_deg_ += inertia_rot_speed_y_ * dt;
        pitch_deg_ = vne::math::clamp(pitch_deg_, kPitchMinDeg, kPitchMaxDeg);
        const float rot_decay = std::exp(-rot_damping_ * dt);
        inertia_rot_speed_x_ *= rot_decay;
        inertia_rot_speed_y_ *= rot_decay;
        changed = true;
    }
    if (inertia_pan_velocity_.length() > kInertiaPanThreshold) {
        coi_world_ += inertia_pan_velocity_ * dt;
        inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
        changed = true;
    }
    if (changed) {
        applyToCamera();
    }
}

void OrbitManipulator::zoomOrthoToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
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
    const vne::math::Vec3f right = computeRight(front);
    const vne::math::Vec3f up = computeUp(front, right);
    const vne::math::Vec3f world_at_cursor = target + right * (ndc_x * half_w) + up * (ndc_y * half_h);
    const float new_half_w = half_w * zoom_factor;
    const float new_half_h = half_h * zoom_factor;
    const vne::math::Vec3f new_target = world_at_cursor - right * (ndc_x * new_half_w) - up * (ndc_y * new_half_h);
    const vne::math::Vec3f eye_offset = eye - target;
    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
    coi_world_ = new_target;
}

void OrbitManipulator::zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    if (!camera_) {
        return;
    }
    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = vne::math::clamp(scene_scale_ * zoom_factor, kSceneScaleMin, kSceneScaleMax);
            return;
        case ZoomMethod::eChangeFov:
            if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(
                    vne::math::clamp(fov * ((zoom_factor < 1.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_),
                                     kFovMinDeg,
                                     kFovMaxDeg));
                persp->updateMatrices();
                return;
            }
            [[fallthrough]];
        case ZoomMethod::eDollyToCoi:
            if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
                zoomOrthoToCursor(zoom_factor, mouse_x_px, mouse_y_px);
                return;
            }
            orbit_distance_ = vne::math::clamp(orbit_distance_ * zoom_factor, kMinOrbitDistance, kMaxOrbitDistance);
            applyToCamera();
            return;
    }
}

void OrbitManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!rotating_ && !panning_) {
        applyInertia(delta_time);
    }
}

void OrbitManipulator::handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (rotating_) {
        dragRotate(delta_x, delta_y, delta_time);
    } else if (panning_) {
        dragPan(x, y, delta_x, delta_y, delta_time);
    }
    last_x_ = x;
    last_y_ = y;
}

void OrbitManipulator::handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    const bool pan_alias = (button == static_cast<int>(MouseButton::eMiddle));
    if (pressed) {
        if (button == button_map_.rotate && !shift_) {
            beginRotate(x, y);
        } else if (button == button_map_.pan || pan_alias || (button == button_map_.rotate && shift_)) {
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

void OrbitManipulator::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    zoom((scroll_y > 0.0f) ? (1.0f / zoom_speed_) : zoom_speed_, mouse_x, mouse_y);
}

void OrbitManipulator::handleKeyboard(int key, bool pressed, double) noexcept {
    if (key == kKeyLeftShift || key == kKeyRightShift) {
        shift_ = pressed;
    }
}

void OrbitManipulator::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    dragRotate(pan.delta_x_px, pan.delta_y_px, delta_time);
}

void OrbitManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) {
        return;
    }
    zoom(1.0f / pinch.scale, pinch.center_x_px, pinch.center_y_px);
}

void OrbitManipulator::resetState() noexcept {
    rotating_ = false;
    panning_ = false;
    inertia_rot_speed_x_ = 0.0f;
    inertia_rot_speed_y_ = 0.0f;
    inertia_pan_velocity_ = vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    shift_ = false;
}

void OrbitManipulator::fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }
    const vne::math::Vec3f center = (min_world + max_world) * 0.5f;
    const vne::math::Vec3f extents = max_world - min_world;
    float radius = extents.length() * 0.5f;
    if (radius < kEpsilon) {
        radius = kMinRadiusFallback;
    }
    coi_world_ = center;
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        const float aspect = std::max(viewport_width_ / viewport_height_, kMinOrthoExtent);
        const float fov_x_rad = 2.0f * vne::math::atan(vne::math::tan(fov_y_rad * 0.5f) * aspect);
        const float dist_y = radius / vne::math::tan(fov_y_rad * 0.5f);
        const float dist_x = radius / vne::math::tan(fov_x_rad * 0.5f);
        orbit_distance_ = std::max(dist_x, dist_y) * kFitToAabbMargin;
        applyToCamera();
    } else if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        const vne::math::Vec3f front = computeFront();
        const vne::math::Vec3f right = computeRight(front);
        const vne::math::Vec3f up = computeUp(front, right);
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
            max_r = std::max(max_r, std::abs(d.dot(right)));
            max_u = std::max(max_u, std::abs(d.dot(up)));
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
        applyToCamera();
    }
}

float OrbitManipulator::getWorldUnitsPerPixel() const noexcept {
    if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        return ortho->getHeight() / viewport_height_;
    }
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        const float fov_y_rad = vne::math::degToRad(persp->getFieldOfView());
        return 2.0f * orbit_distance_ * vne::math::tan(fov_y_rad * 0.5f) / viewport_height_;
    }
    return 0.0f;
}

}  // namespace vne::interaction
