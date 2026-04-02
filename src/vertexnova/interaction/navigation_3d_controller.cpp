/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/navigation_3d_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/free_look_manipulator.h"

#include "camera_controller_context.h"
#include "vertexnova/events/key_event.h"

#include <vertexnova/logging/logging.h>

#include <algorithm>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.navigation_3d");
}  // namespace

namespace vne::interaction {

using namespace vne;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct Navigation3DController::Impl {
    CameraControllerContext core;
    std::shared_ptr<FreeLookManipulator> free_look;  // shared ownership; also in rig

    NavigateMode mode = NavigateMode::eFps;

    KeyBinding move_forward_{events::KeyCode::eW, events::ModifierKey::eModNone};
    KeyBinding move_backward_{events::KeyCode::eS, events::ModifierKey::eModNone};
    KeyBinding move_left_{events::KeyCode::eA, events::ModifierKey::eModNone};
    KeyBinding move_right_{events::KeyCode::eD, events::ModifierKey::eModNone};
    KeyBinding move_up_{events::KeyCode::eE, events::ModifierKey::eModNone};
    KeyBinding move_down_{events::KeyCode::eQ, events::ModifierKey::eModNone};

    events::KeyCode speed_boost_key_ = events::KeyCode::eUnknown;
    events::KeyCode slow_key_ = events::KeyCode::eUnknown;
    MouseBinding look_bind_{MouseButton::eRight, events::ModifierKey::eModNone};
    events::ModifierKey zoom_scroll_modifier_ = events::ModifierKey::eModNone;
    bool look_enabled_ = true;
    bool move_enabled_ = true;
    bool zoom_enabled_ = true;

    events::KeyCode increase_move_speed_key_ = events::KeyCode::eUnknown;
    events::KeyCode decrease_move_speed_key_ = events::KeyCode::eUnknown;
    float move_speed_step_ = 0.25f;
    float move_speed_min_ = 0.1f;
    float move_speed_max_ = 1000.0f;
};

namespace {
InputRule makeButtonRule(
    int button, int mod, CameraActionType press, CameraActionType release, CameraActionType delta) {
    InputRule r;
    r.trigger = InputRule::Trigger::eMouseButton;
    r.code = button;
    r.modifier_mask = mod;
    r.on_press = press;
    r.on_release = release;
    r.on_delta = delta;
    return r;
}

InputRule makeKeyRule(int key, int mod, CameraActionType press, CameraActionType release) {
    InputRule r;
    r.trigger = InputRule::Trigger::eKey;
    r.code = key;
    r.modifier_mask = mod;
    r.on_press = press;
    r.on_release = release;
    return r;
}
}  // namespace

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

Navigation3DController::Navigation3DController()
    : impl_(std::make_unique<Impl>()) {
    rebuild();
}

Navigation3DController::~Navigation3DController() = default;
Navigation3DController::Navigation3DController(Navigation3DController&&) noexcept = default;
Navigation3DController& Navigation3DController::operator=(Navigation3DController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void Navigation3DController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    if (!camera) {
        VNE_LOG_DEBUG << "Navigation3DController: camera detached (null camera)";
    }
    impl_->core.setCamera(std::move(camera));
}

void Navigation3DController::onResize(float w, float h) noexcept {
    const float clamped_w = (w < 1.0f) ? 1.0f : w;
    const float clamped_h = (h < 1.0f) ? 1.0f : h;
    impl_->core.onResize(clamped_w, clamped_h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void Navigation3DController::onEvent(const events::Event& event, double delta_time) noexcept {
    // Key events are unique to Navigation3D — handle before delegating mouse/touch
    switch (event.type()) {
        case events::EventType::eKeyPressed:
        case events::EventType::eKeyRepeat: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->core.mapper.onKey(static_cast<int>(e.keyCode()), true, delta_time);
            return;
        }
        case events::EventType::eKeyReleased: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->core.mapper.onKey(static_cast<int>(e.keyCode()), false, delta_time);
            return;
        }
        default:
            break;
    }
    dispatchMouseEvents(impl_->core.mapper, impl_->core.cursor, event, delta_time);
}

void Navigation3DController::onUpdate(double dt) noexcept {
    impl_->core.onUpdate(dt);
}

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------

void Navigation3DController::setMode(NavigateMode mode) noexcept {
    impl_->mode = mode;
    rebuild();
}

NavigateMode Navigation3DController::getMode() const noexcept {
    return impl_->mode;
}

// ---------------------------------------------------------------------------
// Speed / sensitivity (delegates to free_look_)
// ---------------------------------------------------------------------------

void Navigation3DController::setMoveSpeed(float s) noexcept {
    if (impl_->free_look) {
        impl_->free_look->setMoveSpeed(s);
    }
}

float Navigation3DController::getMoveSpeed() const noexcept {
    return impl_->free_look ? impl_->free_look->getMoveSpeed() : 3.0f;
}

void Navigation3DController::setMouseSensitivity(float s) noexcept {
    if (impl_->free_look) {
        impl_->free_look->setMouseSensitivity(s);
    }
}

float Navigation3DController::getMouseSensitivity() const noexcept {
    return impl_->free_look ? impl_->free_look->getMouseSensitivity() : 0.15f;
}

void Navigation3DController::setSprintMultiplier(float m) noexcept {
    if (impl_->free_look) {
        impl_->free_look->setSprintMultiplier(m);
    }
}

float Navigation3DController::getSprintMultiplier() const noexcept {
    return impl_->free_look ? impl_->free_look->getSprintMultiplier() : 4.0f;
}

void Navigation3DController::setSlowMultiplier(float m) noexcept {
    if (impl_->free_look) {
        impl_->free_look->setSlowMultiplier(m);
    }
}

float Navigation3DController::getSlowMultiplier() const noexcept {
    return impl_->free_look ? impl_->free_look->getSlowMultiplier() : 0.2f;
}

void Navigation3DController::setMoveForwardKey(vne::events::KeyCode key) noexcept {
    impl_->move_forward_.key = key;
    rebuild();
}
void Navigation3DController::setMoveBackwardKey(vne::events::KeyCode key) noexcept {
    impl_->move_backward_.key = key;
    rebuild();
}
void Navigation3DController::setMoveLeftKey(vne::events::KeyCode key) noexcept {
    impl_->move_left_.key = key;
    rebuild();
}
void Navigation3DController::setMoveRightKey(vne::events::KeyCode key) noexcept {
    impl_->move_right_.key = key;
    rebuild();
}
void Navigation3DController::setMoveUpKey(vne::events::KeyCode key) noexcept {
    impl_->move_up_.key = key;
    rebuild();
}
void Navigation3DController::setMoveDownKey(vne::events::KeyCode key) noexcept {
    impl_->move_down_.key = key;
    rebuild();
}

void Navigation3DController::setSpeedBoostKey(vne::events::KeyCode key) noexcept {
    impl_->speed_boost_key_ = key;
    rebuild();
}
void Navigation3DController::setSlowKey(vne::events::KeyCode key) noexcept {
    impl_->slow_key_ = key;
    rebuild();
}
void Navigation3DController::setLookButton(MouseButton btn, vne::events::ModifierKey modifier) noexcept {
    impl_->look_bind_ = MouseBinding{btn, modifier};
    rebuild();
}

void Navigation3DController::setLookEnabled(bool enabled) noexcept {
    impl_->look_enabled_ = enabled;
    rebuild();
}
void Navigation3DController::setMoveEnabled(bool enabled) noexcept {
    impl_->move_enabled_ = enabled;
    rebuild();
}
void Navigation3DController::setZoomEnabled(bool enabled) noexcept {
    impl_->zoom_enabled_ = enabled;
    rebuild();
}
bool Navigation3DController::isLookEnabled() const noexcept {
    return impl_->look_enabled_;
}
bool Navigation3DController::isMoveEnabled() const noexcept {
    return impl_->move_enabled_;
}
bool Navigation3DController::isZoomEnabled() const noexcept {
    return impl_->zoom_enabled_;
}

void Navigation3DController::setIncreaseMoveSpeedKey(vne::events::KeyCode key) noexcept {
    impl_->increase_move_speed_key_ = key;
    rebuild();
}
void Navigation3DController::setDecreaseMoveSpeedKey(vne::events::KeyCode key) noexcept {
    impl_->decrease_move_speed_key_ = key;
    rebuild();
}
void Navigation3DController::setMoveSpeedStep(float delta) noexcept {
    impl_->move_speed_step_ = std::max(0.001f, delta);
}
void Navigation3DController::setMoveSpeedMin(float min_speed) noexcept {
    impl_->move_speed_min_ = std::max(0.001f, min_speed);
    if (impl_->move_speed_max_ < impl_->move_speed_min_) {
        impl_->move_speed_max_ = impl_->move_speed_min_;
    }
}
void Navigation3DController::setMoveSpeedMax(float max_speed) noexcept {
    impl_->move_speed_max_ = std::max(impl_->move_speed_min_, max_speed);
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Navigation3DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->free_look) {
        impl_->free_look->fitToAABB(mn, mx);
    }
}

void Navigation3DController::reset() noexcept {
    impl_->core.resetRigAndInteraction();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& Navigation3DController::inputMapper() noexcept {
    return impl_->core.mapper;
}
FreeLookManipulator& Navigation3DController::freeLookManipulator() noexcept {
    return *impl_->free_look;
}
OrbitalCameraManipulator* Navigation3DController::orbitalCameraManipulator() noexcept {
    return nullptr;
}

// ---------------------------------------------------------------------------
// Private rebuild — called on construction and mode switch
// ---------------------------------------------------------------------------

void Navigation3DController::rebuild() noexcept {
    // Snapshot current speed settings before clearing
    float move_speed = impl_->free_look ? impl_->free_look->getMoveSpeed() : 3.0f;
    float sensitivity = impl_->free_look ? impl_->free_look->getMouseSensitivity() : 0.15f;
    float sprint_mult = impl_->free_look ? impl_->free_look->getSprintMultiplier() : 4.0f;
    float slow_mult = impl_->free_look ? impl_->free_look->getSlowMultiplier() : 0.2f;

    impl_->core.rig.clearManipulators();
    impl_->free_look = nullptr;

    impl_->free_look = std::make_shared<FreeLookManipulator>();
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
    }

    impl_->core.rig.addManipulator(impl_->free_look);

    // Re-attach camera and viewport
    if (impl_->core.camera) {
        impl_->core.rig.setCamera(impl_->core.camera);
    }
    impl_->core.rig.onResize(impl_->core.viewport_w, impl_->core.viewport_h);

    std::vector<InputRule> rules;
    if (impl_->look_enabled_) {
        rules.push_back(makeButtonRule(static_cast<int>(impl_->look_bind_.button),
                                       static_cast<int>(impl_->look_bind_.modifier_mask),
                                       CameraActionType::eBeginLook,
                                       CameraActionType::eEndLook,
                                       CameraActionType::eLookDelta));
    }
    if (impl_->move_enabled_) {
        if (impl_->move_forward_.key != events::KeyCode::eUnknown) {
            rules.push_back(makeKeyRule(static_cast<int>(impl_->move_forward_.key),
                                        static_cast<int>(impl_->move_forward_.modifier_mask),
                                        CameraActionType::eMoveForward,
                                        CameraActionType::eMoveForward));
        }
        if (impl_->move_backward_.key != events::KeyCode::eUnknown) {
            rules.push_back(makeKeyRule(static_cast<int>(impl_->move_backward_.key),
                                        static_cast<int>(impl_->move_backward_.modifier_mask),
                                        CameraActionType::eMoveBackward,
                                        CameraActionType::eMoveBackward));
        }
        if (impl_->move_left_.key != events::KeyCode::eUnknown) {
            rules.push_back(makeKeyRule(static_cast<int>(impl_->move_left_.key),
                                        static_cast<int>(impl_->move_left_.modifier_mask),
                                        CameraActionType::eMoveLeft,
                                        CameraActionType::eMoveLeft));
        }
        if (impl_->move_right_.key != events::KeyCode::eUnknown) {
            rules.push_back(makeKeyRule(static_cast<int>(impl_->move_right_.key),
                                        static_cast<int>(impl_->move_right_.modifier_mask),
                                        CameraActionType::eMoveRight,
                                        CameraActionType::eMoveRight));
        }
        if (impl_->move_up_.key != events::KeyCode::eUnknown) {
            rules.push_back(makeKeyRule(static_cast<int>(impl_->move_up_.key),
                                        static_cast<int>(impl_->move_up_.modifier_mask),
                                        CameraActionType::eMoveUp,
                                        CameraActionType::eMoveUp));
        }
        if (impl_->move_down_.key != events::KeyCode::eUnknown) {
            rules.push_back(makeKeyRule(static_cast<int>(impl_->move_down_.key),
                                        static_cast<int>(impl_->move_down_.modifier_mask),
                                        CameraActionType::eMoveDown,
                                        CameraActionType::eMoveDown));
        }
    }
    if (impl_->speed_boost_key_ == events::KeyCode::eUnknown) {
        rules.push_back(makeKeyRule(static_cast<int>(events::KeyCode::eLeftShift),
                                    kModNone,
                                    CameraActionType::eSprintModifier,
                                    CameraActionType::eSprintModifier));
        rules.push_back(makeKeyRule(static_cast<int>(events::KeyCode::eRightShift),
                                    kModNone,
                                    CameraActionType::eSprintModifier,
                                    CameraActionType::eSprintModifier));
    } else {
        rules.push_back(makeKeyRule(static_cast<int>(impl_->speed_boost_key_),
                                    kModNone,
                                    CameraActionType::eSprintModifier,
                                    CameraActionType::eSprintModifier));
    }
    if (impl_->slow_key_ == events::KeyCode::eUnknown) {
        rules.push_back(makeKeyRule(static_cast<int>(events::KeyCode::eLeftControl),
                                    kModNone,
                                    CameraActionType::eSlowModifier,
                                    CameraActionType::eSlowModifier));
        rules.push_back(makeKeyRule(static_cast<int>(events::KeyCode::eRightControl),
                                    kModNone,
                                    CameraActionType::eSlowModifier,
                                    CameraActionType::eSlowModifier));
    } else {
        rules.push_back(makeKeyRule(static_cast<int>(impl_->slow_key_),
                                    kModNone,
                                    CameraActionType::eSlowModifier,
                                    CameraActionType::eSlowModifier));
    }
    if (impl_->zoom_enabled_) {
        InputRule z;
        z.trigger = InputRule::Trigger::eScroll;
        z.modifier_mask = static_cast<int>(impl_->zoom_scroll_modifier_);
        z.on_delta = CameraActionType::eZoomAtCursor;
        rules.push_back(z);
        rules.push_back({.trigger = InputRule::Trigger::eTouchPinch, .on_delta = CameraActionType::eZoomAtCursor});
    }
    if (impl_->increase_move_speed_key_ != events::KeyCode::eUnknown) {
        rules.push_back(makeKeyRule(static_cast<int>(impl_->increase_move_speed_key_),
                                    kModNone,
                                    CameraActionType::eIncreaseMoveSpeed,
                                    CameraActionType::eNone));
    }
    if (impl_->decrease_move_speed_key_ != events::KeyCode::eUnknown) {
        rules.push_back(makeKeyRule(static_cast<int>(impl_->decrease_move_speed_key_),
                                    kModNone,
                                    CameraActionType::eDecreaseMoveSpeed,
                                    CameraActionType::eNone));
    }
    impl_->core.mapper.setRules(rules);
    // Capture raw Impl* so the callback stays valid across moves.
    impl_->core.mapper.setActionCallback([impl = impl_.get()](CameraActionType a, const CameraCommandPayload& p, double dt) {
        if (a == CameraActionType::eIncreaseMoveSpeed && p.pressed) {
            if (impl->free_look) {
                const float current = impl->free_look->getMoveSpeed();
                impl->free_look->setMoveSpeed(
                    std::clamp(current + impl->move_speed_step_, impl->move_speed_min_, impl->move_speed_max_));
            }
            return;
        }
        if (a == CameraActionType::eDecreaseMoveSpeed && p.pressed) {
            if (impl->free_look) {
                const float current = impl->free_look->getMoveSpeed();
                impl->free_look->setMoveSpeed(
                    std::clamp(current - impl->move_speed_step_, impl->move_speed_min_, impl->move_speed_max_));
            }
            return;
        }
        impl->core.rig.onAction(a, p, dt);
        // fpsPreset() does not emit orbit gestures (eBeginRotate / eBeginPan). Scroll and touch pinch map to
        // eZoomAtCursor; after zoom/dolly the camera pose changes—mark yaw/pitch stale so FreeLook's next
        // ensureAnglesSynced (update / movement / look) matches the rig.
        if (impl->free_look && a == CameraActionType::eZoomAtCursor) {
            impl->free_look->markAnglesDirty();
        }
    });
}

}  // namespace vne::interaction
