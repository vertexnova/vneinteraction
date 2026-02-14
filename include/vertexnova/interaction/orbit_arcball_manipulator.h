#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include <vertexnova/math/core/core.h>
#include <memory>

namespace vne::interaction {

class OrbitArcballManipulator final : public ICameraManipulator {
public:
    OrbitArcballManipulator() noexcept;
    ~OrbitArcballManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }
    void setViewportSize(float width_px, float height_px) noexcept override;
    void update(double delta_time) noexcept override;

    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    void handleMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    [[nodiscard]] float getSceneScale() const noexcept override { return scene_scale_; }

    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    void setViewDirection(ViewDirection dir) noexcept;
    void setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept;
    [[nodiscard]] vne::math::Vec3f getCenterOfInterestWorld() const noexcept { return coi_world_; }
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }
    void setOrbitDistance(float distance) noexcept;
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = speed; }
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }
    void setPanSpeed(float speed) noexcept { pan_speed_ = speed; }
    [[nodiscard]] float getPanSpeed() const noexcept { return pan_speed_; }

private:
    [[nodiscard]] bool isPerspective() const noexcept;
    [[nodiscard]] bool isOrthographic() const noexcept;
    void syncFromCamera() noexcept;
    void applyToCamera() noexcept;
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept;
    [[nodiscard]] vne::math::Vec3f computeRight(const vne::math::Vec3f& front) const noexcept;
    [[nodiscard]] vne::math::Vec3f computeUp(const vne::math::Vec3f& front, const vne::math::Vec3f& right) const noexcept;
    [[nodiscard]] vne::math::Vec3f projectToArcballWorld(float x_px, float y_px) const noexcept;
    void beginRotate(float x_px, float y_px) noexcept;
    void dragRotate(float x_px, float y_px, double delta_time) noexcept;
    void endRotate(double) noexcept;
    void beginPan(float, float) noexcept;
    void dragPan(float, float, float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void endPan(double) noexcept;
    void applyInertia(double delta_time) noexcept;
    void zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    vne::math::Vec3f world_up_;
    vne::math::Vec3f coi_world_;
    float orbit_distance_ = 5.0f;
    float scene_scale_ = 1.0f;
    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
    bool rotating_ = false;
    bool panning_ = false;
    float last_x_ = 0.0f;
    float last_y_ = 0.0f;
    vne::math::Vec3f arcball_start_world_;
    vne::math::Vec3f eye_start_offset_;
    vne::math::Vec3f up_start_;
    float inertia_rot_speed_ = 0.0f;
    vne::math::Vec3f inertia_rot_axis_;
    vne::math::Vec3f inertia_pan_velocity_;
    float rot_damping_ = 8.0f;
    float pan_damping_ = 10.0f;
    float rotation_speed_ = 1.0f;
    float pan_speed_ = 1.0f;
    bool shift_ = false;
};

}  // namespace vne::interaction
