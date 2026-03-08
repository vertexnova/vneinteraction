/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/angle_utils.h"
#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne::math;

namespace {
constexpr float kEpsilon = 1e-6f;
float safeSqrt(float x) noexcept {
    return std::sqrt(std::max(0.0f, x));
}
float scrollToZoomFactor(float scroll_y, float zoom_speed) noexcept {
    return (scroll_y > 0.0f) ? (1.0f / zoom_speed) : zoom_speed;
}
}  // namespace

ArcballManipulator::ArcballManipulator() noexcept
    : world_up_(0.0f, 1.0f, 0.0f)
    , coi_world_(0.0f, 0.0f, 0.0f)
    , inertia_rot_axis_(0.0f, 1.0f, 0.0f)
    , inertia_pan_velocity_(0.0f, 0.0f, 0.0f) {}

void ArcballManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncFromCamera();
}

void ArcballManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        persp->setViewport(viewport_width_, viewport_height_);
    }
}

bool ArcballManipulator::isPerspective() const noexcept {
    return static_cast<bool>(std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_));
}

bool ArcballManipulator::isOrthographic() const noexcept {
    return static_cast<bool>(std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_));
}

Vec3f ArcballManipulator::computeFront() const noexcept {
    if (!camera_) {
        return Vec3f(0.0f, 0.0f, -1.0f);
    }
    Vec3f dir = camera_->getTarget() - camera_->getPosition();
    const float len = dir.length();
    return (len < kEpsilon) ? Vec3f(0.0f, 0.0f, -1.0f) : (dir / len);
}

Vec3f ArcballManipulator::computeRight(const Vec3f& front) const noexcept {
    Vec3f r = world_up_.cross(front);
    float len = r.length();
    if (len < kEpsilon) {
        const Vec3f fallback_up = (std::abs(front.z()) < 0.999f) ? Vec3f(0.0f, 0.0f, 1.0f) : Vec3f(0.0f, 1.0f, 0.0f);
        r = fallback_up.cross(front);
        len = r.length();
    }
    return (len < kEpsilon) ? Vec3f(1.0f, 0.0f, 0.0f) : (r / len);
}

Vec3f ArcballManipulator::computeUp(const Vec3f& front, const Vec3f& right) const noexcept {
    Vec3f up = front.cross(right);
    const float len = up.length();
    return (len < kEpsilon) ? world_up_ : (up / len);
}

void ArcballManipulator::setWorldUp(const Vec3f& world_up) noexcept {
    if (world_up.length() < kEpsilon) {
        return;
    }
    world_up_ = world_up.normalized();
}

void ArcballManipulator::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = std::clamp(distance, 0.01f, 1e6f);
    applyToCamera();
}

void ArcballManipulator::setCenterOfInterest(const Vec3f& coi, CenterOfInterestSpace space) noexcept {
    if (!camera_) {
        coi_world_ = coi;
        return;
    }

    if (space == CenterOfInterestSpace::eWorldSpace) {
        coi_world_ = coi;
    } else {
        const Vec3f front = computeFront();
        const Vec3f r = computeRight(front);
        const Vec3f u = computeUp(front, r);
        coi_world_ = camera_->getPosition() + r * coi.x() + u * coi.y() + front * coi.z();
    }

    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), 0.01f);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
}

void ArcballManipulator::resetState() noexcept {
    rotating_ = false;
    panning_ = false;
    inertia_rot_speed_ = 0.0f;
    inertia_rot_axis_ = Vec3f(0.0f, 1.0f, 0.0f);
    inertia_pan_velocity_ = Vec3f(0.0f, 0.0f, 0.0f);
    last_x_ = 0.0f;
    last_y_ = 0.0f;
    shift_ = false;
}

void ArcballManipulator::syncFromCamera() noexcept {
    if (!camera_) {
        return;
    }
    coi_world_ = camera_->getTarget();
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), 0.01f);
}

void ArcballManipulator::applyToCamera() noexcept {
    if (!camera_) {
        return;
    }
    Vec3f dir = camera_->getPosition() - coi_world_;
    const float len = dir.length();
    dir = (len < kEpsilon) ? Vec3f(0.0f, 0.0f, 1.0f) : (dir / len);
    camera_->setPosition(coi_world_ + dir * orbit_distance_);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
}

Vec3f ArcballManipulator::projectToArcball(float x_px, float y_px) const noexcept {
    const float half_size = 0.5f * std::min(viewport_width_, viewport_height_);
    const float cx = viewport_width_ * 0.5f;
    const float cy = viewport_height_ * 0.5f;

    float ax = (x_px - cx) / half_size;
    float ay = (cy - y_px) / half_size;
    const float r2 = ax * ax + ay * ay;
    float az = 0.0f;

    if (r2 <= 1.0f) {
        az = safeSqrt(1.0f - r2);
    } else {
        const float inv_len = 1.0f / std::sqrt(r2);
        ax *= inv_len;
        ay *= inv_len;
    }

    const Vec3f front = computeFront();
    const Vec3f r = computeRight(front);
    const Vec3f u = computeUp(front, r);
    return (r * ax + u * ay + front * az).normalized();
}

void ArcballManipulator::beginRotate(float x_px, float y_px) noexcept {
    if (!camera_) {
        return;
    }
    rotating_ = true;
    arcball_start_world_ = projectToArcball(x_px, y_px);
    inertia_rot_speed_ = 0.0f;
}

void ArcballManipulator::dragRotate(float x_px, float y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }

    const Vec3f prev = arcball_start_world_;
    const Vec3f curr = projectToArcball(x_px, y_px);
    arcball_start_world_ = curr;

    const float dot = std::clamp(prev.dot(curr), -1.0f, 1.0f);
    if (dot >= 1.0f - kEpsilon) {
        return;
    }

    Vec3f axis = prev.cross(curr);
    const float axis_len = axis.length();
    if (axis_len < kEpsilon) {
        return;
    }
    axis /= axis_len;

    const float angle = std::acos(dot) * rotation_speed_;
    const Quatf q = Quatf::fromAxisAngle(axis, angle);

    const Vec3f eye_offset = camera_->getPosition() - coi_world_;
    const Vec3f up = camera_->getUp();
    camera_->setPosition(coi_world_ + q.rotate(eye_offset));
    camera_->setUp(q.rotate(up));
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
    orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), 0.01f);

    if (delta_time > 0.0 && std::abs(angle) > 1e-6f) {
        inertia_rot_axis_ = axis;
        inertia_rot_speed_ = std::clamp(angle / static_cast<float>(delta_time), -10.0f, 10.0f);
    }
}

void ArcballManipulator::endRotate(double) noexcept {
    rotating_ = false;
}

void ArcballManipulator::beginPan(float x_px, float y_px) noexcept {
    panning_ = true;
    last_x_ = x_px;
    last_y_ = y_px;
    inertia_pan_velocity_ = Vec3f(0.0f, 0.0f, 0.0f);
}

void ArcballManipulator::dragPan(float, float, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    if (!camera_) {
        return;
    }

    const Vec3f front = computeFront();
    const Vec3f r = computeRight(front);
    const Vec3f u = computeUp(front, r);

    Vec3f delta_world(0.0f, 0.0f, 0.0f);
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        (void)persp;
        const float wpp = getWorldUnitsPerPixel();
        delta_world = r * (-delta_x_px * wpp * pan_speed_) + u * (delta_y_px * wpp * pan_speed_);
    } else if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        const float wppx = ortho->getWidth() / viewport_width_;
        const float wppy = ortho->getHeight() / viewport_height_;
        delta_world = r * (-delta_x_px * wppx * pan_speed_) + u * (delta_y_px * wppy * pan_speed_);
    }

    coi_world_ += delta_world;
    camera_->setPosition(camera_->getPosition() + delta_world);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();

    if (delta_time > 0.0) {
        inertia_pan_velocity_ = delta_world / static_cast<float>(delta_time);
    }
}

void ArcballManipulator::endPan(double) noexcept {
    panning_ = false;
}

void ArcballManipulator::applyInertia(double delta_time) noexcept {
    if (!camera_ || delta_time <= 0.0) {
        return;
    }

    const float dt = static_cast<float>(delta_time);
    bool changed = false;

    if (std::abs(inertia_rot_speed_) > 1e-4f) {
        const Quatf q = Quatf::fromAxisAngle(inertia_rot_axis_, inertia_rot_speed_ * dt);
        const Vec3f eye_offset = camera_->getPosition() - coi_world_;
        const Vec3f up = camera_->getUp();
        camera_->setPosition(coi_world_ + q.rotate(eye_offset));
        camera_->setUp(q.rotate(up));
        camera_->setTarget(coi_world_);
        inertia_rot_speed_ *= std::exp(-rot_damping_ * dt);
        changed = true;
    }

    if (inertia_pan_velocity_.length() > 1e-4f) {
        const Vec3f delta = inertia_pan_velocity_ * dt;
        coi_world_ += delta;
        camera_->setPosition(camera_->getPosition() + delta);
        camera_->setTarget(coi_world_);
        inertia_pan_velocity_ *= std::exp(-pan_damping_ * dt);
        changed = true;
    }

    if (changed) {
        orbit_distance_ = std::max((camera_->getPosition() - coi_world_).length(), 0.01f);
        camera_->updateMatrices();
    }
}

void ArcballManipulator::zoomOrthoToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_);
    if (!ortho) {
        return;
    }

    const float ndc_x = (2.0f * mouse_x_px / viewport_width_) - 1.0f;
    const float ndc_y = 1.0f - (2.0f * mouse_y_px / viewport_height_);
    const float half_w = ortho->getWidth() * 0.5f;
    const float half_h = ortho->getHeight() * 0.5f;
    const Vec3f eye = ortho->getPosition();
    const Vec3f target = ortho->getTarget();
    Vec3f front = target - eye;
    const float front_len = front.length();
    front = (front_len < kEpsilon) ? Vec3f(0.0f, 0.0f, -1.0f) : (front / front_len);
    const Vec3f r = computeRight(front);
    const Vec3f u = computeUp(front, r);
    const Vec3f world_at_cursor = target + r * (ndc_x * half_w) + u * (ndc_y * half_h);
    const float new_half_w = half_w * zoom_factor;
    const float new_half_h = half_h * zoom_factor;
    const Vec3f new_target = world_at_cursor - r * (ndc_x * new_half_w) - u * (ndc_y * new_half_h);
    const Vec3f eye_offset = eye - target;
    ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
    ortho->setTarget(new_target);
    ortho->setPosition(new_target + eye_offset);
    ortho->updateMatrices();
    coi_world_ = new_target;
}

void ArcballManipulator::zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    if (!camera_) {
        return;
    }

    switch (zoom_method_) {
        case ZoomMethod::eSceneScale:
            scene_scale_ = std::clamp(scene_scale_ * zoom_factor, 1e-4f, 1e4f);
            return;
        case ZoomMethod::eChangeFov:
            if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
                const float fov = persp->getFieldOfView();
                persp->setFieldOfView(
                    std::clamp(fov * ((zoom_factor < 1.0f) ? (1.0f / fov_zoom_speed_) : fov_zoom_speed_),
                               5.0f,
                               120.0f));
                persp->updateMatrices();
                return;
            }
            [[fallthrough]];
        case ZoomMethod::eDollyToCoi:
            if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
                (void)ortho;
                zoomOrthoToCursor(zoom_factor, mouse_x_px, mouse_y_px);
                return;
            }
            orbit_distance_ = std::clamp(orbit_distance_ * zoom_factor, 0.01f, 1e6f);
            applyToCamera();
            return;
    }
}

void ArcballManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!rotating_ && !panning_) {
        applyInertia(delta_time);
    }
}

void ArcballManipulator::handleMouseMove(float x, float y, float, float, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (rotating_) {
        dragRotate(x, y, delta_time);
    } else if (panning_) {
        dragPan(x, y, x - last_x_, y - last_y_, delta_time);
    }
    last_x_ = x;
    last_y_ = y;
}

void ArcballManipulator::handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
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

void ArcballManipulator::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) {
        return;
    }
    zoom(scrollToZoomFactor(scroll_y, zoom_speed_), mouse_x, mouse_y);
}

void ArcballManipulator::handleKeyboard(int key, bool pressed, double) noexcept {
    if (key == 340 || key == 344) {
        shift_ = pressed;
    }
}

void ArcballManipulator::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) {
        return;
    }
    if (!rotating_) {
        beginRotate(viewport_width_ * 0.5f, viewport_height_ * 0.5f);
    }
    dragRotate(last_x_ + pan.delta_x_px, last_y_ + pan.delta_y_px, delta_time);
}

void ArcballManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) {
        return;
    }
    zoom(1.0f / pinch.scale, pinch.center_x_px, pinch.center_y_px);
}

void ArcballManipulator::fitToAABB(const Vec3f& min_world, const Vec3f& max_world) noexcept {
    if (!camera_) {
        return;
    }

    const Vec3f center = (min_world + max_world) * 0.5f;
    const Vec3f extents = max_world - min_world;
    float radius = extents.length() * 0.5f;
    if (radius < kEpsilon) {
        radius = 1.0f;
    }

    coi_world_ = center;

    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        const float fov_y_rad = degToRad(persp->getFieldOfView());
        const float aspect = std::max(viewport_width_ / viewport_height_, 1e-3f);
        const float fov_x_rad = 2.0f * std::atan(std::tan(fov_y_rad * 0.5f) * aspect);
        const float dist_y = radius / std::tan(fov_y_rad * 0.5f);
        const float dist_x = radius / std::tan(fov_x_rad * 0.5f);
        orbit_distance_ = std::max(dist_x, dist_y) * 1.1f;
        applyToCamera();
    } else if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        const Vec3f front = computeFront();
        const Vec3f r = computeRight(front);
        const Vec3f u = computeUp(front, r);
        const Vec3f corners[8] = {
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
            const Vec3f d = c - center;
            max_r = std::max(max_r, std::abs(d.dot(r)));
            max_u = std::max(max_u, std::abs(d.dot(u)));
        }
        max_r = std::max(max_r * 1.1f, 1e-3f);
        max_u = std::max(max_u * 1.1f, 1e-3f);
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

float ArcballManipulator::getWorldUnitsPerPixel() const noexcept {
    if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        return ortho->getHeight() / viewport_height_;
    }
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        return 2.0f * orbit_distance_ * std::tan(degToRad(persp->getFieldOfView()) * 0.5f) / viewport_height_;
    }
    return 0.0f;
}

}  // namespace vne::interaction
