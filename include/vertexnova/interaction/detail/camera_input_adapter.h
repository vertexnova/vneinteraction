#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @brief Translates raw input (mouse, keyboard, touch) into semantic camera
 * commands and forwards them to a manipulator. Decouples input bindings from
 * manipulator logic so bindings can be remapped without changing manipulators.
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>

namespace vne::interaction {

class CameraInputAdapter {
   public:
    CameraInputAdapter() = default;
    explicit CameraInputAdapter(std::shared_ptr<ICameraManipulator> manipulator) noexcept
        : manipulator_(std::move(manipulator)) {}

    void setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept {
        manipulator_ = std::move(manipulator);
        current_action_ = Action::None;
    }
    [[nodiscard]] std::shared_ptr<ICameraManipulator> getManipulator() const noexcept { return manipulator_; }

    void feedMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept;
    void feedMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept;
    void feedMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept;
    void feedKeyboard(int key, bool pressed, double delta_time) noexcept;
    void feedTouchPan(const TouchPan& pan, double delta_time) noexcept;
    void feedTouchPinch(const TouchPinch& pinch, double delta_time) noexcept;

   private:
    enum class Action { None, Rotate, Pan, Look };

    void send(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept;

    std::shared_ptr<ICameraManipulator> manipulator_;
    Action current_action_ = Action::None;
    float last_x_px_ = 0.0f;
    float last_y_px_ = 0.0f;
    bool modifier_shift_ = false;

    // Default bindings (e.g. GLFW-like)
    int rotate_button_ = static_cast<int>(MouseButton::eLeft);
    int pan_button_ = static_cast<int>(MouseButton::eRight);
    int look_button_ = static_cast<int>(MouseButton::eRight);
    int key_w_ = 87, key_s_ = 83, key_a_ = 65, key_d_ = 68, key_q_ = 81, key_e_ = 69;
    int key_shift_left_ = 340, key_shift_right_ = 344;
    int key_ctrl_left_ = 341, key_ctrl_right_ = 345;
};

}  // namespace vne::interaction
