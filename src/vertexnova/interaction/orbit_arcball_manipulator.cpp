/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbit_arcball_manipulator.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include <vertexnova/math/core/core.h>
#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne::math;

namespace {
constexpr float kEpsilon = 1e-6f;
float safeSqrt(float x) noexcept { return std::sqrt(std::max(0.0f, x)); }
}  // namespace

OrbitArcballManipulator::OrbitArcballManipulator() noexcept
    : world_up_(0.0f, 1.0f, 0.0f), coi_world_(0.0f, 0.0f, 0.0f), inertia_rot_axis_(0.0f, 1.0f, 0.0f),
      inertia_pan_velocity_(0.0f, 0.0f, 0.0f) {}

void OrbitArcballManipulator::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    syncFromCamera();
}

void OrbitArcballManipulator::setViewportSize(float width_px, float height_px) noexcept {
    viewport_width_ = std::max(1.0f, width_px);
    viewport_height_ = std::max(1.0f, height_px);
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        persp->setViewport(viewport_width_, viewport_height_);
    }
}

bool OrbitArcballManipulator::isPerspective() const noexcept {
    return static_cast<bool>(std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_));
}

bool OrbitArcballManipulator::isOrthographic() const noexcept {
    return static_cast<bool>(std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_));
}

Vec3f OrbitArcballManipulator::computeFront() const noexcept {
    if (!camera_) return Vec3f(0.0f, 0.0f, -1.0f);
    Vec3f dir = camera_->getTarget() - camera_->getPosition();
    float len = dir.length();
    if (len < kEpsilon) return Vec3f(0.0f, 0.0f, -1.0f);
    return dir / len;
}

Vec3f OrbitArcballManipulator::computeRight(const Vec3f& front) const noexcept {
    Vec3f r = front.cross(world_up_);
    float len = r.length();
    if (len < kEpsilon) r = front.cross(Vec3f(0.0f, 0.0f, 1.0f));
    len = r.length();
    if (len < kEpsilon) return Vec3f(1.0f, 0.0f, 0.0f);
    return r / len;
}

Vec3f OrbitArcballManipulator::computeUp(const Vec3f& front, const Vec3f& right) const noexcept {
    Vec3f up = right.cross(front);
    float len = up.length();
    if (len < kEpsilon) return world_up_;
    return up / len;
}

void OrbitArcballManipulator::setWorldUp(const Vec3f& world_up) noexcept {
    if (world_up.length() < kEpsilon) return;
    world_up_ = world_up.normalized();
}

void OrbitArcballManipulator::setOrbitDistance(float distance) noexcept {
    orbit_distance_ = std::clamp(distance, 0.01f, 1e6f);
    applyToCamera();
}

void OrbitArcballManipulator::setViewDirection(ViewDirection dir) noexcept {
    if (!camera_) return;
    Vec3f d(0.0f, 0.0f, 1.0f);
    switch (dir) {
        case ViewDirection::eFront: d = Vec3f(0.0f, 0.0f, 1.0f); break;
        case ViewDirection::eBack: d = Vec3f(0.0f, 0.0f, -1.0f); break;
        case ViewDirection::eLeft: d = Vec3f(-1.0f, 0.0f, 0.0f); break;
        case ViewDirection::eRight: d = Vec3f(1.0f, 0.0f, 0.0f); break;
        case ViewDirection::eTop: d = Vec3f(0.0f, 1.0f, 0.0f); break;
        case ViewDirection::eBottom: d = Vec3f(0.0f, -1.0f, 0.0f); break;
        case ViewDirection::eIso: d = Vec3f(1.0f, 1.0f, 1.0f); break;
    }
    d = d.normalized();
    Vec3f eye = coi_world_ + d * orbit_distance_;
    camera_->setPosition(eye);
    camera_->setTarget(coi_world_);
    camera_->setUp(world_up_);
    camera_->updateMatrices();
}

void OrbitArcballManipulator::setCenterOfInterest(const Vec3f& coi, CenterOfInterestSpace space) noexcept {
    if (!camera_) {
        coi_world_ = coi;
        return;
    }
    if (space == CenterOfInterestSpace::eWorldSpace) {
        coi_world_ = coi;
    } else {
        Vec3f front = computeFront();
        Vec3f r = computeRight(front);
        Vec3f u = computeUp(front, r);
        coi_world_ = camera_->getPosition() + r * coi.x() + u * coi.y() + front * coi.z();
    }
    Vec3f eye = camera_->getPosition();
    orbit_distance_ = (eye - coi_world_).length();
    if (orbit_distance_ < 0.01f) orbit_distance_ = 0.01f;
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
}

void OrbitArcballManipulator::syncFromCamera() noexcept {
    if (!camera_) return;
    coi_world_ = camera_->getTarget();
    Vec3f eye = camera_->getPosition();
    orbit_distance_ = (eye - coi_world_).length();
    if (orbit_distance_ < 0.01f) orbit_distance_ = 0.01f;
}

void OrbitArcballManipulator::applyToCamera() noexcept {
    if (!camera_) return;
    Vec3f dir = camera_->getPosition() - coi_world_;
    float len = dir.length();
    if (len < kEpsilon) dir = Vec3f(0.0f, 0.0f, 1.0f);
    else dir = dir / len;
    Vec3f eye = coi_world_ + dir * orbit_distance_;
    camera_->setPosition(eye);
    camera_->setTarget(coi_world_);
    camera_->updateMatrices();
}

Vec3f OrbitArcballManipulator::projectToArcballWorld(float x_px, float y_px) const noexcept {
    if (!camera_) return Vec3f(0.0f, 0.0f, 1.0f);
    float nx = (2.0f * x_px / viewport_width_) - 1.0f;
    float ny = 1.0f - (2.0f * y_px / viewport_height_);
    float aspect = viewport_width_ / viewport_height_;
    float ax = nx, ay = ny;
    if (aspect > 1.0f) ax *= aspect;
    else ay /= aspect;
    float r2 = ax * ax + ay * ay;
    float az = 0.0f;
    if (r2 <= 1.0f) az = safeSqrt(1.0f - r2);
    else {
        float inv_len = 1.0f / std::sqrt(r2);
        ax *= inv_len;
        ay *= inv_len;
    }
    Vec3f v_cam(ax, ay, az);
    v_cam = v_cam.normalized();
    Vec3f front = computeFront();
    Vec3f r = computeRight(front);
    Vec3f u = computeUp(front, r);
    Vec3f v_world = r * v_cam.x() + u * v_cam.y() + front * v_cam.z();
    return v_world.normalized();
}

void OrbitArcballManipulator::beginRotate(float x_px, float y_px) noexcept {
    if (!camera_) return;
    rotating_ = true;
    arcball_start_world_ = projectToArcballWorld(x_px, y_px);
    eye_start_offset_ = camera_->getPosition() - coi_world_;
    up_start_ = camera_->getUp();
    inertia_rot_speed_ = 0.0f;
}

void OrbitArcballManipulator::dragRotate(float x_px, float y_px, double delta_time) noexcept {
    if (!camera_) return;
    Vec3f curr = projectToArcballWorld(x_px, y_px);
    Quatf q;
    q.makeRotate(arcball_start_world_, curr);
    float angle = 0.0f;
    Vec3f axis(0.0f, 1.0f, 0.0f);
    q.getAngleAndAxis(angle, axis);
    angle *= rotation_speed_;
    q = Quatf::fromAxisAngle(axis, angle);
    Vec3f eye_offset = q.rotate(eye_start_offset_);
    Vec3f up = q.rotate(up_start_);
    camera_->setPosition(coi_world_ + eye_offset);
    camera_->setTarget(coi_world_);
    camera_->setUp(up);
    camera_->updateMatrices();
    if (delta_time > 0.0 && std::abs(angle) > 1e-6f) {
        inertia_rot_axis_ = axis.normalized();
        inertia_rot_speed_ = std::clamp(static_cast<float>(angle / delta_time), -10.0f, 10.0f);
    }
}

void OrbitArcballManipulator::endRotate(double) noexcept { rotating_ = false; }

void OrbitArcballManipulator::beginPan(float, float) noexcept {
    panning_ = true;
    inertia_pan_velocity_ = Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitArcballManipulator::dragPan(float, float, float delta_x_px, float delta_y_px, double delta_time) noexcept {
    if (!camera_) return;
    Vec3f front = computeFront();
    Vec3f r = computeRight(front);
    Vec3f u = computeUp(front, r);
    Vec3f delta_world(0.0f, 0.0f, 0.0f);
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        float fov_rad = degToRad(persp->getFieldOfView());
        float depth = std::max(0.01f, orbit_distance_);
        float world_per_pixel = (2.0f * depth * std::tan(fov_rad * 0.5f)) / viewport_height_;
        delta_world = r * (-delta_x_px * world_per_pixel) + u * (delta_y_px * world_per_pixel);
    } else if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        float wppx = ortho->getWidth() / viewport_width_;
        float wppy = ortho->getHeight() / viewport_height_;
        delta_world = r * (-delta_x_px * wppx) + u * (delta_y_px * wppy);
    }
    coi_world_ = coi_world_ + delta_world;
    camera_->setTarget(coi_world_);
    camera_->setPosition(camera_->getPosition() + delta_world);
    camera_->updateMatrices();
    if (delta_time > 0.0)
        inertia_pan_velocity_ = delta_world / static_cast<float>(delta_time);
}

void OrbitArcballManipulator::endPan(double) noexcept { panning_ = false; }

void OrbitArcballManipulator::applyInertia(double delta_time) noexcept {
    if (!camera_ || delta_time <= 0.0) return;
    if (std::abs(inertia_rot_speed_) > 1e-4f) {
        float dt = static_cast<float>(delta_time);
        Quatf q = Quatf::fromAxisAngle(inertia_rot_axis_, inertia_rot_speed_ * dt);
        Vec3f eye_offset = camera_->getPosition() - coi_world_;
        Vec3f up = camera_->getUp();
        camera_->setPosition(coi_world_ + q.rotate(eye_offset));
        camera_->setUp(q.rotate(up));
        camera_->setTarget(coi_world_);
        camera_->updateMatrices();
        inertia_rot_speed_ *= std::exp(-rot_damping_ * dt);
    } else inertia_rot_speed_ = 0.0f;
    if (inertia_pan_velocity_.length() > 1e-4f) {
        float dt = static_cast<float>(delta_time);
        Vec3f delta = inertia_pan_velocity_ * dt;
        coi_world_ = coi_world_ + delta;
        camera_->setTarget(coi_world_);
        camera_->setPosition(camera_->getPosition() + delta);
        camera_->updateMatrices();
        inertia_pan_velocity_ = inertia_pan_velocity_ * std::exp(-pan_damping_ * dt);
    } else inertia_pan_velocity_ = Vec3f(0.0f, 0.0f, 0.0f);
}

void OrbitArcballManipulator::zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept {
    if (!camera_) return;
    zoom_factor = std::clamp(zoom_factor, 0.01f, 100.0f);
    if (zoom_method_ == ZoomMethod::eDollyToCoi && isPerspective()) {
        orbit_distance_ = std::clamp(orbit_distance_ * zoom_factor, 0.01f, 1e6f);
        applyToCamera();
        return;
    }
    if (zoom_method_ == ZoomMethod::eSceneScale) {
        scene_scale_ = std::clamp(scene_scale_ * zoom_factor, 1e-4f, 1e4f);
        return;
    }
    if (auto persp = std::dynamic_pointer_cast<vne::scene::PerspectiveCamera>(camera_)) {
        float fov = persp->getFieldOfView();
        persp->setFieldOfView(std::clamp(fov * zoom_factor, 5.0f, 120.0f));
        persp->updateMatrices();
        return;
    }
    if (auto ortho = std::dynamic_pointer_cast<vne::scene::OrthographicCamera>(camera_)) {
        float ndc_x = (2.0f * mouse_x_px / viewport_width_) - 1.0f;
        float ndc_y = 1.0f - (2.0f * mouse_y_px / viewport_height_);
        float half_w = ortho->getWidth() * 0.5f;
        float half_h = ortho->getHeight() * 0.5f;
        Vec3f front = computeFront();
        Vec3f r = computeRight(front);
        Vec3f u = computeUp(front, r);
        Vec3f world_at_cursor = coi_world_ + r * (ndc_x * half_w) + u * (ndc_y * half_h);
        float new_half_w = half_w * zoom_factor;
        float new_half_h = half_h * zoom_factor;
        coi_world_ = world_at_cursor - r * (ndc_x * new_half_w) - u * (ndc_y * new_half_h);
        ortho->setBounds(-new_half_w, new_half_w, -new_half_h, new_half_h, ortho->getNearPlane(), ortho->getFarPlane());
        camera_->setTarget(coi_world_);
        camera_->setPosition(coi_world_ - front * orbit_distance_);
        camera_->updateMatrices();
    }
}

void OrbitArcballManipulator::update(double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    if (!rotating_ && !panning_) applyInertia(delta_time);
}

void OrbitArcballManipulator::handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    if (rotating_) dragRotate(x, y, delta_time);
    else if (panning_) dragPan(x, y, delta_x, delta_y, delta_time);
    last_x_ = x;
    last_y_ = y;
}

void OrbitArcballManipulator::handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    if (button == 0) {
        if (pressed) beginRotate(x, y);
        else endRotate(delta_time);
    }
    if (button == 1 || button == 2) {
        if (pressed) beginPan(x, y);
        else endPan(delta_time);
    }
    last_x_ = x;
    last_y_ = y;
}

void OrbitArcballManipulator::handleMouseScroll(float, float scroll_y, float mouse_x, float mouse_y, double) noexcept {
    if (!enabled_ || !camera_ || scroll_y == 0.0f) return;
    float zoom_factor = (scroll_y > 0.0f) ? 0.9f : 1.1f;
    zoom(zoom_factor, mouse_x, mouse_y);
}

void OrbitArcballManipulator::handleKeyboard(int key, bool pressed, double) noexcept {
    if (key == 340 || key == 344) shift_ = pressed;
}

void OrbitArcballManipulator::handleTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!enabled_ || !camera_) return;
    dragPan(last_x_, last_y_, pan.delta_x_px, pan.delta_y_px, delta_time);
}

void OrbitArcballManipulator::handleTouchPinch(const TouchPinch& pinch, double) noexcept {
    if (!enabled_ || !camera_ || pinch.scale <= 0.0f) return;
    zoom(1.0f / pinch.scale, pinch.center_x_px, pinch.center_y_px);
}

}  // namespace vne::interaction
