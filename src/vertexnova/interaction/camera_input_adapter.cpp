/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/detail/camera_input_adapter.h"

namespace vne::interaction {

void CameraInputAdapter::send(CameraActionType action,
                              const CameraCommandPayload& payload,
                              double delta_time) noexcept {
    if (manipulator_) {
        manipulator_->applyCommand(action, payload, delta_time);
    }
}

void CameraInputAdapter::feedMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept {
    last_x_px_ = x;
    last_y_px_ = y;
    if (!manipulator_) {
        return;
    }
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;
    payload.delta_x_px = delta_x;
    payload.delta_y_px = delta_y;
    if (current_action_ == Action::Rotate) {
        send(CameraActionType::eRotateDelta, payload, delta_time);
    } else if (current_action_ == Action::Pan) {
        send(CameraActionType::ePanDelta, payload, delta_time);
    } else if (current_action_ == Action::Look) {
        send(CameraActionType::eLookDelta, payload, delta_time);
    }
}

void CameraInputAdapter::feedMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept {
    last_x_px_ = x;
    last_y_px_ = y;
    if (!manipulator_) {
        return;
    }
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;
    payload.pressed = pressed;

    const bool is_rotate_btn = (button == bindings_.rotate_button);
    const bool is_pan_btn = (button == bindings_.pan_button || button == bindings_.pan_button_middle);
    const bool is_look = (button == bindings_.look_button);
    const bool orbit_pan_alias = is_rotate_btn && modifier_shift_;

    if (pressed) {
        if (is_rotate_btn && !orbit_pan_alias) {
            current_action_ = Action::Rotate;
            send(CameraActionType::eBeginRotate, payload, delta_time);
        } else if (is_pan_btn || orbit_pan_alias) {
            current_action_ = Action::Pan;
            send(CameraActionType::eBeginPan, payload, delta_time);
        } else if (is_look) {
            current_action_ = Action::Look;
            send(CameraActionType::eBeginLook, payload, delta_time);
        }
    } else {
        if (button == bindings_.rotate_button) {
            if (current_action_ == Action::Rotate) {
                send(CameraActionType::eEndRotate, payload, delta_time);
            } else if (current_action_ == Action::Pan) {
                send(CameraActionType::eEndPan, payload, delta_time);
            }
            if (current_action_ == Action::Rotate || current_action_ == Action::Pan) {
                current_action_ = Action::None;
            }
        }
        if (is_pan_btn && button != bindings_.rotate_button) {
            if (current_action_ == Action::Pan) {
                send(CameraActionType::eEndPan, payload, delta_time);
                current_action_ = Action::None;
            }
        }
        if (button == bindings_.look_button) {
            if (current_action_ == Action::Look) {
                send(CameraActionType::eEndLook, payload, delta_time);
            }
            current_action_ = Action::None;
        }
    }
}

void CameraInputAdapter::feedMouseScroll(
    float /* scroll_x */, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept {
    if (!manipulator_ || scroll_y == 0.0f) {
        return;
    }
    const float zoom_speed = manipulator_->getZoomSpeed();
    CameraCommandPayload payload;
    payload.x_px = mouse_x;
    payload.y_px = mouse_y;
    payload.zoom_factor = (scroll_y > 0.0f) ? (1.0f / zoom_speed) : zoom_speed;
    send(CameraActionType::eZoomAtCursor, payload, delta_time);
}

void CameraInputAdapter::feedKeyboard(int key, bool pressed, double delta_time) noexcept {
    if (!manipulator_) {
        return;
    }
    CameraCommandPayload payload;
    payload.pressed = pressed;
    if (key == bindings_.key_shift_left || key == bindings_.key_shift_right) {
        modifier_shift_ = pressed;
        send(CameraActionType::eOrbitPanModifier, payload, delta_time);
        send(CameraActionType::eSprintModifier, payload, delta_time);
    } else if (key == bindings_.key_ctrl_left || key == bindings_.key_ctrl_right) {
        send(CameraActionType::eSlowModifier, payload, delta_time);
    } else if (key == bindings_.key_move_forward) {
        send(CameraActionType::eMoveForward, payload, delta_time);
    } else if (key == bindings_.key_move_backward) {
        send(CameraActionType::eMoveBackward, payload, delta_time);
    } else if (key == bindings_.key_move_left) {
        send(CameraActionType::eMoveLeft, payload, delta_time);
    } else if (key == bindings_.key_move_right) {
        send(CameraActionType::eMoveRight, payload, delta_time);
    } else if (key == bindings_.key_move_up) {
        send(CameraActionType::eMoveUp, payload, delta_time);
    } else if (key == bindings_.key_move_down) {
        send(CameraActionType::eMoveDown, payload, delta_time);
    }
}

void CameraInputAdapter::feedTouchPan(const TouchPan& pan, double delta_time) noexcept {
    if (!manipulator_) {
        return;
    }
    last_x_px_ += pan.delta_x_px;
    last_y_px_ += pan.delta_y_px;
    CameraCommandPayload payload;
    payload.x_px = last_x_px_;
    payload.y_px = last_y_px_;
    payload.delta_x_px = pan.delta_x_px;
    payload.delta_y_px = pan.delta_y_px;
    // Ortho-only manipulators pan on touch; orbit-style manipulators rotate
    const bool is_ortho_pan = manipulator_->supportsOrthographic() && !manipulator_->supportsPerspective();
    if (is_ortho_pan) {
        const bool has_delta = (pan.delta_x_px != 0.0f) || (pan.delta_y_px != 0.0f);
        if (has_delta) {
            if (!touch_pan_active_) {
                CameraCommandPayload begin_payload;
                begin_payload.x_px = last_x_px_;
                begin_payload.y_px = last_y_px_;
                send(CameraActionType::eBeginPan, begin_payload, delta_time);
                touch_pan_active_ = true;
            }
            send(CameraActionType::ePanDelta, payload, delta_time);
        } else if (touch_pan_active_) {
            send(CameraActionType::eEndPan, payload, delta_time);
            touch_pan_active_ = false;
        }
    } else {
        if (touch_pan_active_) {
            send(CameraActionType::eEndPan, payload, delta_time);
            touch_pan_active_ = false;
        }
        send(CameraActionType::eRotateDelta, payload, delta_time);
    }
}

void CameraInputAdapter::feedTouchPinch(const TouchPinch& pinch, double delta_time) noexcept {
    if (!manipulator_ || pinch.scale <= 0.0f) {
        return;
    }
    CameraCommandPayload payload;
    payload.x_px = pinch.center_x_px;
    payload.y_px = pinch.center_y_px;
    payload.zoom_factor = 1.0f / pinch.scale;
    send(CameraActionType::eZoomAtCursor, payload, delta_time);
}

}  // namespace vne::interaction
