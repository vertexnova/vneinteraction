#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/detail/camera_manipulator_base.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

/**
 * Non-exported base for free-look manipulators (FPS, Fly).
 * Shared WASD movement, look, zoom, and input handling.
 * Subclasses implement upVector() (FPS: world_up_, Fly: camera up).
 */
class FreeCameraBase : public CameraManipulatorBase {
   protected:
    bool looking_ = false;
    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float move_speed_ = 3.0f;
    float mouse_sensitivity_ = 0.15f;
    float fov_zoom_speed_ = 1.05f;
    float sprint_mult_ = 4.0f;
    float slow_mult_ = 0.2f;
    bool w_ = false;
    bool a_ = false;
    bool s_ = false;
    bool d_ = false;
    bool q_ = false;
    bool e_ = false;
    bool sprint_ = false;
    bool slow_ = false;

    FreeCameraBase() = default;
    ~FreeCameraBase() override = default;

    [[nodiscard]] virtual vne::math::Vec3f upVector() const noexcept = 0;

    [[nodiscard]] vne::math::Vec3f front() const noexcept;
    [[nodiscard]] vne::math::Vec3f right(const vne::math::Vec3f& front_vec) const noexcept;
    void syncAnglesFromCamera() noexcept;
    void applyAnglesToCamera() noexcept;
    void applyZoom(float zoom_step_or_factor) noexcept;

   public:
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void update(double delta_time) noexcept override;
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;
};

}  // namespace vne::interaction
