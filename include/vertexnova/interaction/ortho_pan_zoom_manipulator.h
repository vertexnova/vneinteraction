#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include <vertexnova/math/core/core.h>
#include <memory>

namespace vne::interaction {

class OrthoPanZoomManipulator final : public ICameraManipulator {
public:
    OrthoPanZoomManipulator() noexcept = default;
    ~OrthoPanZoomManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return false; }
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

    void setPanDamping(float damping) noexcept { pan_damping_ = damping; }
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

private:
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> getOrtho() const noexcept;
    void pan(float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void zoomToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void applyInertia(double delta_time) noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    bool panning_ = false;
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};
    float pan_damping_ = 10.0f;
};

}  // namespace vne::interaction
