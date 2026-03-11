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

class VNE_INTERACTION_API OrbitManipulator final : public OrbitStyleBase {
   public:
    OrbitManipulator() noexcept;
    ~OrbitManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    void resetState() noexcept override;
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    void setViewDirection(ViewDirection dir) noexcept;
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
    void syncFromCamera() noexcept override;
    void applyToCamera() noexcept override;
    void beginRotate(float x_px, float y_px) noexcept override;
    void dragRotate(float delta_x_px, float delta_y_px, double delta_time) noexcept override;
    void endRotate(double delta_time) noexcept override;
    void applyInertia(double delta_time) noexcept override;

    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float inertia_rot_speed_x_ = 0.0f;
    float inertia_rot_speed_y_ = 0.0f;
};

}  // namespace vne::interaction
