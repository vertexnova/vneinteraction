#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include <vertexnova/math/core/core.h>
#include <memory>

namespace vne::interaction {

class FpsFlyManipulator final : public ICameraManipulator {
public:
    FpsFlyManipulator() noexcept = default;
    ~FpsFlyManipulator() noexcept override = default;

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

    void setMoveSpeed(float units_per_sec) noexcept { move_speed_ = units_per_sec; }
    void setMouseSensitivity(float deg_per_pixel) noexcept { mouse_sensitivity_ = deg_per_pixel; }

private:
    void syncAnglesFromCamera() noexcept;
    void applyAnglesToCamera() noexcept;
    [[nodiscard]] vne::math::Vec3f front() const noexcept;
    [[nodiscard]] vne::math::Vec3f right(const vne::math::Vec3f& front) const noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    bool looking_ = false;
    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float move_speed_ = 3.0f;
    float mouse_sensitivity_ = 0.15f;
    bool w_ = false, a_ = false, s_ = false, d_ = false;
    bool q_ = false, e_ = false;
};

}  // namespace vne::interaction
