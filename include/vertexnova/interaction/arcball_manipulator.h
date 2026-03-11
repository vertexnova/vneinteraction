#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/detail/orbit_style_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

class VNE_INTERACTION_API ArcballManipulator final : public OrbitStyleBase {
   public:
    ArcballManipulator() noexcept;
    ~ArcballManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setViewportSize(float width_px, float height_px) noexcept override;
    void applyCommand(CameraActionType action,
                      const CameraCommandPayload& payload,
                      double delta_time) noexcept override;

    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;

    void resetState() noexcept override;
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    void setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept;
    [[nodiscard]] vne::math::Vec3f getCenterOfInterestWorld() const noexcept { return coi_world_; }
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }
    void setOrbitDistance(float distance) noexcept;
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = std::max(0.0f, speed); }
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }
    void setPanSpeed(float speed) noexcept { pan_speed_ = std::max(0.0f, speed); }
    [[nodiscard]] float getPanSpeed() const noexcept { return pan_speed_; }
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    void setFovZoomSpeed(float speed) noexcept { fov_zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }
    void setRotationDamping(float damping) noexcept { rot_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getRotationDamping() const noexcept { return rot_damping_; }
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }
    void setButtonMap(const ButtonMap& map) noexcept { button_map_ = map; }
    [[nodiscard]] const ButtonMap& getButtonMap() const noexcept { return button_map_; }

   private:
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept override;
    // Projects screen pixel to a unit vector in camera-space (pure screen-space, no world vectors).
    [[nodiscard]] vne::math::Vec3f projectToArcball(float x_px, float y_px) const noexcept;
    void syncFromCamera() noexcept override;
    void applyToCamera() noexcept override;
    void beginRotate(float x_px, float y_px) noexcept override;
    void dragRotate(float x_px, float y_px, double delta_time) noexcept override;
    void endRotate(double delta_time) noexcept override;
    void applyInertia(double delta_time) noexcept override;
    void onPivotChanged() noexcept override;

    // Screen-space drag start (replaces world-space arcball_start_world_ which caused drift)
    float arcball_start_x_ = 0.0f;
    float arcball_start_y_ = 0.0f;

    // Persistent accumulated rotation quaternion — canonical rotation state, prevents float drift
    vne::math::Quatf orientation_;
    uint32_t normalize_counter_ = 0;

    float inertia_rot_speed_ = 0.0f;
    vne::math::Vec3f inertia_rot_axis_;
};

}  // namespace vne::interaction
