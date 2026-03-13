/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/navigate_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/interaction/orbit_behavior.h"

#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/touch_event.h"

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct NavigateController::Impl {
    CameraRig rig;
    InputMapper mapper;
    std::shared_ptr<FreeLookBehavior> free_look;  // shared ownership; also in rig
    std::shared_ptr<OrbitBehavior> orbit;         // shared ownership; only in eGame mode

    NavigateMode mode = NavigateMode::eFps;

    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;

    bool first_mouse = true;
    double last_x = 0.0;
    double last_y = 0.0;
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

NavigateController::NavigateController()
    : impl_(std::make_unique<Impl>()) {
    rebuild();
}

NavigateController::~NavigateController() = default;
NavigateController::NavigateController(NavigateController&&) noexcept = default;
NavigateController& NavigateController::operator=(NavigateController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void NavigateController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    impl_->camera = camera;
    impl_->rig.setCamera(camera);
}

void NavigateController::setViewportSize(float w, float h) noexcept {
    impl_->viewport_w = w;
    impl_->viewport_h = h;
    impl_->rig.setViewportSize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void NavigateController::onEvent(const vne::events::Event& event, double delta_time) noexcept {
    using ET = vne::events::EventType;

    switch (event.type()) {
        case ET::eMouseMoved: {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            const float x = static_cast<float>(e.x());
            const float y = static_cast<float>(e.y());
            const float dx = impl_->first_mouse ? 0.0f : static_cast<float>(e.x() - impl_->last_x);
            const float dy = impl_->first_mouse ? 0.0f : static_cast<float>(e.y() - impl_->last_y);
            impl_->last_x = e.x();
            impl_->last_y = e.y();
            impl_->first_mouse = false;
            impl_->mapper.onMouseMove(x, y, dx, dy, delta_time);
            break;
        }
        case ET::eMouseButtonPressed: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                impl_->last_x = e.x();
                impl_->last_y = e.y();
            }
            impl_->first_mouse = false;
            impl_->mapper.onMouseButton(static_cast<int>(e.button()),
                                        true,
                                        static_cast<float>(impl_->last_x),
                                        static_cast<float>(impl_->last_y),
                                        delta_time);
            break;
        }
        case ET::eMouseButtonReleased: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                impl_->last_x = e.x();
                impl_->last_y = e.y();
            }
            impl_->mapper.onMouseButton(static_cast<int>(e.button()),
                                        false,
                                        static_cast<float>(impl_->last_x),
                                        static_cast<float>(impl_->last_y),
                                        delta_time);
            break;
        }
        case ET::eMouseButtonDoubleClicked: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                impl_->last_x = e.x();
                impl_->last_y = e.y();
            }
            impl_->first_mouse = false;
            impl_->mapper.onMouseDoubleClick(static_cast<int>(e.button()),
                                             static_cast<float>(impl_->last_x),
                                             static_cast<float>(impl_->last_y),
                                             delta_time);
            break;
        }
        case ET::eMouseScrolled: {
            const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
            impl_->mapper.onMouseScroll(static_cast<float>(e.xOffset()),
                                        static_cast<float>(e.yOffset()),
                                        static_cast<float>(impl_->last_x),
                                        static_cast<float>(impl_->last_y),
                                        delta_time);
            break;
        }
        case ET::eKeyPressed:
        case ET::eKeyRepeat: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            impl_->mapper.onKey(static_cast<int>(e.keyCode()), true, delta_time);
            break;
        }
        case ET::eKeyReleased: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            impl_->mapper.onKey(static_cast<int>(e.keyCode()), false, delta_time);
            break;
        }
        case ET::eTouchPress: {
            const auto& e = static_cast<const vne::events::TouchPressEvent&>(event);
            impl_->last_x = e.x();
            impl_->last_y = e.y();
            impl_->first_mouse = false;
            break;
        }
        case ET::eTouchMove: {
            const auto& e = static_cast<const vne::events::TouchMoveEvent&>(event);
            const float dx = impl_->first_mouse ? 0.0f : static_cast<float>(e.x() - impl_->last_x);
            const float dy = impl_->first_mouse ? 0.0f : static_cast<float>(e.y() - impl_->last_y);
            impl_->last_x = e.x();
            impl_->last_y = e.y();
            impl_->first_mouse = false;
            impl_->mapper.onTouchPan(TouchPan{dx, dy}, delta_time);
            break;
        }
        case ET::eTouchRelease:
            impl_->first_mouse = true;
            break;
        default:
            break;
    }
}

void NavigateController::update(double dt) noexcept {
    impl_->rig.update(dt);
}

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------

void NavigateController::setMode(NavigateMode mode) noexcept {
    impl_->mode = mode;
    rebuild();
}

NavigateMode NavigateController::getMode() const noexcept {
    return impl_->mode;
}

// ---------------------------------------------------------------------------
// Speed / sensitivity (delegates to free_look_)
// ---------------------------------------------------------------------------

void NavigateController::setMoveSpeed(float s) noexcept {
    if (impl_->free_look)
        impl_->free_look->setMoveSpeed(s);
}

float NavigateController::getMoveSpeed() const noexcept {
    return impl_->free_look ? impl_->free_look->getMoveSpeed() : 3.0f;
}

void NavigateController::setMouseSensitivity(float s) noexcept {
    if (impl_->free_look)
        impl_->free_look->setMouseSensitivity(s);
}

float NavigateController::getMouseSensitivity() const noexcept {
    return impl_->free_look ? impl_->free_look->getMouseSensitivity() : 0.15f;
}

void NavigateController::setSprintMultiplier(float m) noexcept {
    if (impl_->free_look)
        impl_->free_look->setSprintMultiplier(m);
}

float NavigateController::getSprintMultiplier() const noexcept {
    return impl_->free_look ? impl_->free_look->getSprintMultiplier() : 4.0f;
}

void NavigateController::setSlowMultiplier(float m) noexcept {
    if (impl_->free_look)
        impl_->free_look->setSlowMultiplier(m);
}

float NavigateController::getSlowMultiplier() const noexcept {
    return impl_->free_look ? impl_->free_look->getSlowMultiplier() : 0.2f;
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void NavigateController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->free_look)
        impl_->free_look->fitToAABB(mn, mx);
}

void NavigateController::reset() noexcept {
    impl_->first_mouse = true;
    impl_->rig.resetState();
    impl_->mapper.resetState();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& NavigateController::inputMapper() noexcept {
    return impl_->mapper;
}
FreeLookBehavior& NavigateController::freeLookBehavior() noexcept {
    return *impl_->free_look;
}
OrbitBehavior* NavigateController::orbitBehavior() noexcept {
    return impl_->orbit.get();
}

// ---------------------------------------------------------------------------
// Private rebuild — called on construction and mode switch
// ---------------------------------------------------------------------------

void NavigateController::rebuild() noexcept {
    // Snapshot current speed settings before clearing
    float move_speed = impl_->free_look ? impl_->free_look->getMoveSpeed() : 3.0f;
    float sensitivity = impl_->free_look ? impl_->free_look->getMouseSensitivity() : 0.15f;
    float sprint_mult = impl_->free_look ? impl_->free_look->getSprintMultiplier() : 4.0f;
    float slow_mult = impl_->free_look ? impl_->free_look->getSlowMultiplier() : 0.2f;

    impl_->rig.clearBehaviors();
    impl_->orbit = nullptr;
    impl_->free_look = nullptr;

    // FreeLookBehavior for all three modes
    impl_->free_look = std::make_shared<FreeLookBehavior>();
    impl_->free_look->setMoveSpeed(move_speed);
    impl_->free_look->setMouseSensitivity(sensitivity);
    impl_->free_look->setSprintMultiplier(sprint_mult);
    impl_->free_look->setSlowMultiplier(slow_mult);

    switch (impl_->mode) {
        case NavigateMode::eFps:
            impl_->free_look->setConstrainWorldUp(true);
            break;
        case NavigateMode::eFly:
            impl_->free_look->setConstrainWorldUp(false);
            break;
        case NavigateMode::eGame: {
            impl_->free_look->setConstrainWorldUp(false);
            impl_->orbit = std::make_shared<OrbitBehavior>();
            impl_->orbit->setRotationMode(OrbitRotationMode::eEuler);
            impl_->rig.addBehavior(impl_->orbit);
            break;
        }
    }

    impl_->rig.addBehavior(impl_->free_look);

    // Re-attach camera and viewport
    if (impl_->camera)
        impl_->rig.setCamera(impl_->camera);
    impl_->rig.setViewportSize(impl_->viewport_w, impl_->viewport_h);

    // Load input preset
    std::vector<InputRule> rules;
    switch (impl_->mode) {
        case NavigateMode::eFps:
        case NavigateMode::eFly:
            rules = InputMapper::fpsPreset();
            break;
        case NavigateMode::eGame:
            rules = InputMapper::gamePreset();
            break;
    }
    impl_->mapper.setRules(rules);
    impl_->mapper.setActionCallback(
        [this](CameraActionType a, const CameraCommandPayload& p, double dt) { impl_->rig.onAction(a, p, dt); });
}

}  // namespace vne::interaction
