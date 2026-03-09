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
#include "vertexnova/interaction/detail/camera_manipulator_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::scene {
class OrthographicCamera;
}

namespace vne::interaction {

class VNE_INTERACTION_API OrthoPanZoomManipulator final : public CameraManipulatorBase {
   public:
    OrthoPanZoomManipulator() noexcept = default;
    ~OrthoPanZoomManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return false; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setViewportSize(float width_px, float height_px) noexcept override;
    void update(double delta_time) noexcept override;

    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    void resetState() noexcept override;
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getSceneScale() const noexcept override { return scene_scale_; }
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

   private:
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> getOrtho() const noexcept;
    void pan(float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void zoomToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void applyInertia(double delta_time) noexcept;
    void applyZoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;

    bool panning_ = false;
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};
    float pan_damping_ = 10.0f;
};

}  // namespace vne::interaction
