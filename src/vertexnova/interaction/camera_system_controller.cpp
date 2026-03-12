/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_system_controller.h"

#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/touch_event.h"

namespace vne::interaction {

void CameraSystemController::assignCameraToManipulator() noexcept {
    if (manipulator_ && camera_) {
        manipulator_->setCamera(camera_);
    }
}

void CameraSystemController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    camera_ = std::move(camera);
    assignCameraToManipulator();
}

void CameraSystemController::setEnabled(bool enabled) noexcept {
    enabled_ = enabled;
    if (manipulator_) {
        manipulator_->setEnabled(enabled);
    }
}

void CameraSystemController::update(double delta_time) noexcept {
    if (manipulator_) {
        manipulator_->update(delta_time);
    }
}

void CameraSystemController::reset() noexcept {
    first_mouse_ = true;
    if (manipulator_) {
        manipulator_->resetState();
    }
}

void CameraSystemController::setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept {
    manipulator_ = std::move(manipulator);
    input_adapter_.setManipulator(manipulator_);
    assignCameraToManipulator();
    if (manipulator_) {
        manipulator_->setEnabled(enabled_);
    }
}

void CameraSystemController::setViewportSize(float width_px, float height_px) noexcept {
    if (manipulator_) {
        manipulator_->setViewportSize(width_px, height_px);
    }
}

void CameraSystemController::onEvent(const vne::events::Event& event, double delta_time) noexcept {
    if (!enabled_ || !manipulator_) {
        return;
    }
    using ET = vne::events::EventType;
    switch (event.type()) {
        case ET::eMouseMoved: {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            const float x = static_cast<float>(e.x());
            const float y = static_cast<float>(e.y());
            const float dx = first_mouse_ ? 0.0f : static_cast<float>(e.x() - last_x_);
            const float dy = first_mouse_ ? 0.0f : static_cast<float>(e.y() - last_y_);
            last_x_ = e.x();
            last_y_ = e.y();
            first_mouse_ = false;
            input_adapter_.onMouseMove(x, y, dx, dy, delta_time);
            break;
        }
        case ET::eMouseButtonPressed: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                last_x_ = e.x();
                last_y_ = e.y();
            }
            first_mouse_ = false;
            input_adapter_.onMouseButton(static_cast<int>(e.button()),
                                         true,
                                         static_cast<float>(last_x_),
                                         static_cast<float>(last_y_),
                                         delta_time);
            break;
        }
        case ET::eMouseButtonReleased: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                last_x_ = e.x();
                last_y_ = e.y();
            }
            input_adapter_.onMouseButton(static_cast<int>(e.button()),
                                         false,
                                         static_cast<float>(last_x_),
                                         static_cast<float>(last_y_),
                                         delta_time);
            break;
        }
        case ET::eMouseScrolled: {
            const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
            input_adapter_.onMouseScroll(static_cast<float>(e.xOffset()),
                                         static_cast<float>(e.yOffset()),
                                         static_cast<float>(last_x_),
                                         static_cast<float>(last_y_),
                                         delta_time);
            break;
        }
        case ET::eKeyPressed:
        case ET::eKeyRepeat: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            input_adapter_.onKeyboard(static_cast<int>(e.keyCode()), true, delta_time);
            break;
        }
        case ET::eKeyReleased: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            input_adapter_.onKeyboard(static_cast<int>(e.keyCode()), false, delta_time);
            break;
        }
        case ET::eTouchPress: {
            const auto& e = static_cast<const vne::events::TouchPressEvent&>(event);
            last_x_ = e.x();
            last_y_ = e.y();
            first_mouse_ = false;
            break;
        }
        case ET::eTouchMove: {
            const auto& e = static_cast<const vne::events::TouchMoveEvent&>(event);
            const float dx = first_mouse_ ? 0.0f : static_cast<float>(e.x() - last_x_);
            const float dy = first_mouse_ ? 0.0f : static_cast<float>(e.y() - last_y_);
            last_x_ = e.x();
            last_y_ = e.y();
            first_mouse_ = false;
            TouchPan pan{dx, dy};
            input_adapter_.onTouchPan(pan, delta_time);
            break;
        }
        case ET::eTouchRelease:
            first_mouse_ = true;
            break;
        default:
            break;
    }
}

void CameraSystemController::setInputBindings(const CameraInputBindings& bindings) noexcept {
    input_adapter_.setBindings(bindings);
}

CameraInputBindings CameraSystemController::getInputBindings() const noexcept {
    return input_adapter_.getBindings();
}

}  // namespace vne::interaction
