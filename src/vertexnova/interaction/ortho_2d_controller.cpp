/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_2d_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"

#include "camera_controller_impl.h"

#include <vertexnova/events/key_event.h>
#include <vertexnova/logging/logging.h>

#include <vector>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.ortho_2d");

[[nodiscard]] bool hasMouseButtonChord(const std::vector<vne::interaction::InputRule>& rules,
                                       int button,
                                       int modifier_mask) noexcept {
    using vne::interaction::InputRule;
    for (const auto& r : rules) {
        if (r.trigger == InputRule::Trigger::eMouseButton && r.code == button && r.modifier_mask == modifier_mask) {
            return true;
        }
    }
    return false;
}
}  // namespace

namespace vne::interaction {

using namespace vne;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct Ortho2DController::Impl {
    CameraControllerContext core_;
    std::shared_ptr<Ortho2DManipulator> ortho2d_behavior_;
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

Ortho2DController::Ortho2DController()
    : impl_(std::make_unique<Impl>()) {
    impl_->ortho2d_behavior_ = std::make_shared<Ortho2DManipulator>();
    impl_->core_.rig.addManipulator(impl_->ortho2d_behavior_);

    // Capture raw Impl* so the callback stays valid across moves.
    impl_->core_.mapper.setActionCallback([impl = impl_.get()](CameraActionType a,
                                                              const CameraCommandPayload& p,
                                                              double dt) { impl->core_.rig.onAction(a, p, dt); });

    rebuildRules();
}

Ortho2DController::~Ortho2DController() = default;
Ortho2DController::Ortho2DController(Ortho2DController&&) noexcept = default;
Ortho2DController& Ortho2DController::operator=(Ortho2DController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void Ortho2DController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    if (!camera) {
        VNE_LOG_DEBUG << "Ortho2DController: camera detached (null camera)";
    }
    impl_->core_.setCamera(std::move(camera));
}

void Ortho2DController::onResize(float w, float h) noexcept {
    impl_->core_.onResize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void Ortho2DController::onEvent(const events::Event& event, double delta_time) noexcept {
    switch (event.type()) {
        case events::EventType::eKeyPressed:
        case events::EventType::eKeyRepeat: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->core_.mapper.onKey(static_cast<int>(e.keyCode()), true, delta_time);
            return;
        }
        case events::EventType::eKeyReleased: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->core_.mapper.onKey(static_cast<int>(e.keyCode()), false, delta_time);
            return;
        }
        default:
            break;
    }
    dispatchMouseEvents(impl_->core_.mapper, impl_->core_.cursor, event, delta_time);
}

void Ortho2DController::onUpdate(double dt) noexcept {
    impl_->core_.onUpdate(dt);
}

// ---------------------------------------------------------------------------
// DOF
// ---------------------------------------------------------------------------

void Ortho2DController::setRotationEnabled(bool enabled) noexcept {
    // End in-flight rotate before clearing rules / manipulator flags. Ortho2DManipulator always clears
    // rotating_ on eEndRotate; eBeginRotate / eRotateDelta remain gated by rotate_enabled_.
    if (!enabled && rotation_enabled_ && impl_->ortho2d_behavior_) {
        const CameraCommandPayload p{};
        impl_->core_.rig.onAction(CameraActionType::eEndRotate, p, 0.0);
    }
    rotation_enabled_ = enabled;
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->setRotateEnabled(enabled);
    }
    rebuildRules();
}

void Ortho2DController::setPanEnabled(bool enabled) noexcept {
    if (!enabled && pan_enabled_ && impl_->ortho2d_behavior_) {
        const CameraCommandPayload p{};
        impl_->core_.rig.onAction(CameraActionType::eEndPan, p, 0.0);
    }
    pan_enabled_ = enabled;
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->setPanEnabled(enabled);
    }
    rebuildRules();
}

void Ortho2DController::setZoomEnabled(bool enabled) noexcept {
    zoom_enabled_ = enabled;
    rebuildRules();
}

void Ortho2DController::setPanButton(MouseButton btn, events::ModifierKey modifier) noexcept {
    pan_binding_ = MouseBinding{btn, modifier};
    rebuildRules();
}

void Ortho2DController::setRotateButton(MouseButton btn, events::ModifierKey modifier) noexcept {
    rotate_binding_ = MouseBinding{btn, modifier};
    rebuildRules();
}

void Ortho2DController::setZoomScrollModifier(events::ModifierKey modifier) noexcept {
    zoom_scroll_modifier_ = modifier;
    rebuildRules();
}

void Ortho2DController::setPanInertiaEnabled(bool enabled) noexcept {
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->setPanInertiaEnabled(enabled);
    }
}

void Ortho2DController::setRotateSensitivity(float degrees_per_pixel) noexcept {
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->setRotationSensitivityDegreesPerPixel(degrees_per_pixel);
    }
}

void Ortho2DController::setZoomSensitivity(float multiplier) noexcept {
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->setZoomSpeed(multiplier);
    }
}

void Ortho2DController::setPanSensitivity(float damping) noexcept {
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->setPanDamping(damping);
    }
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Ortho2DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->ortho2d_behavior_) {
        impl_->ortho2d_behavior_->fitToAABB(mn, mx);
    }
}

void Ortho2DController::reset() noexcept {
    impl_->core_.resetRigAndInteraction();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& Ortho2DController::inputMapper() noexcept {
    return impl_->core_.mapper;
}
Ortho2DManipulator& Ortho2DController::ortho2DManipulator() noexcept {
    return *impl_->ortho2d_behavior_;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void Ortho2DController::rebuildRules() noexcept {
    std::vector<InputRule> rules;

    if (pan_enabled_) {
        const int pan_btn = static_cast<int>(pan_binding_.button);
        const int pan_mod = static_cast<int>(pan_binding_.modifier_mask);
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = pan_btn,
            .modifier_mask = pan_mod,
            .on_press = CameraActionType::eBeginPan,
            .on_release = CameraActionType::eEndPan,
            .on_delta = CameraActionType::ePanDelta,
        });
        const int mid_btn = static_cast<int>(MouseButton::eMiddle);
        if (!hasMouseButtonChord(rules, mid_btn, kModNone)) {
            rules.push_back({
                .trigger = InputRule::Trigger::eMouseButton,
                .code = mid_btn,
                .modifier_mask = kModNone,
                .on_press = CameraActionType::eBeginPan,
                .on_release = CameraActionType::eEndPan,
                .on_delta = CameraActionType::ePanDelta,
            });
        }
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPan,
            .on_delta = CameraActionType::ePanDelta,
        });
    }

    if (zoom_enabled_) {
        rules.push_back({
            .trigger = InputRule::Trigger::eScroll,
            .modifier_mask = static_cast<int>(zoom_scroll_modifier_),
            .on_delta = CameraActionType::eZoomAtCursor,
        });
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPinch,
            .on_delta = CameraActionType::eZoomAtCursor,
        });
    }

    if (rotation_enabled_) {
        const int rot_btn = static_cast<int>(rotate_binding_.button);
        const int rot_mod = static_cast<int>(rotate_binding_.modifier_mask);
        if (!hasMouseButtonChord(rules, rot_btn, rot_mod)) {
            rules.push_back({
                .trigger = InputRule::Trigger::eMouseButton,
                .code = rot_btn,
                .modifier_mask = rot_mod,
                .on_press = CameraActionType::eBeginRotate,
                .on_release = CameraActionType::eEndRotate,
                .on_delta = CameraActionType::eRotateDelta,
            });
        }
    }

    impl_->core_.mapper.setRules(rules);
}

}  // namespace vne::interaction
