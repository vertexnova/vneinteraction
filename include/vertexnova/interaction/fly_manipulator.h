#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

class VNE_INTERACTION_API FlyManipulator final : public ICameraManipulator {
   public:
    FlyManipulator() noexcept = default;
    ~FlyManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }
    void setViewportSize(float width_px, float height_px) noexcept override;
    void update(double delta_time) noexcept override;

    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    void resetState() noexcept override;
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getSceneScale() const noexcept override { return scene_scale_; }
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    void setMoveSpeed(float units_per_sec) noexcept { move_speed_ = std::max(0.0f, units_per_sec); }
    [[nodiscard]] float getMoveSpeed() const noexcept { return move_speed_; }
    void setMouseSensitivity(float deg_per_pixel) noexcept { mouse_sensitivity_ = std::max(0.0f, deg_per_pixel); }
    [[nodiscard]] float getMouseSensitivity() const noexcept { return mouse_sensitivity_; }
    void setZoomSpeed(float units_or_factor) noexcept { zoom_speed_ = std::max(0.01f, units_or_factor); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }
    void setFovZoomSpeed(float factor) noexcept { fov_zoom_speed_ = std::max(0.01f, factor); }
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }
    void setSprintMultiplier(float mult) noexcept { sprint_mult_ = std::max(1.0f, mult); }
    [[nodiscard]] float getSprintMultiplier() const noexcept { return sprint_mult_; }
    void setSlowMultiplier(float mult) noexcept { slow_mult_ = std::clamp(mult, 0.0f, 1.0f); }
    [[nodiscard]] float getSlowMultiplier() const noexcept { return slow_mult_; }

   private:
    void syncAnglesFromCamera() noexcept;
    void applyAnglesToCamera() noexcept;
    [[nodiscard]] vne::math::Vec3f front() const noexcept;
    [[nodiscard]] vne::math::Vec3f right(const vne::math::Vec3f& front) const noexcept;
    [[nodiscard]] vne::math::Vec3f upAxis() const noexcept;
    void applyZoom(float zoom_step_or_factor) noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    bool looking_ = false;
    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float move_speed_ = 3.0f;
    float mouse_sensitivity_ = 0.15f;
    float zoom_speed_ = 0.5f;
    float fov_zoom_speed_ = 1.05f;
    float sprint_mult_ = 4.0f;
    float slow_mult_ = 0.2f;
    float scene_scale_ = 1.0f;
    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
    bool w_ = false;
    bool a_ = false;
    bool s_ = false;
    bool d_ = false;
    bool q_ = false;
    bool e_ = false;
    bool sprint_ = false;
    bool slow_ = false;
};

}  // namespace vne::interaction
