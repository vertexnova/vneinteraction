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

#include "controller_event_dispatch.h"

#include <vertexnova/logging/logging.h>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.ortho_2d");
}  // namespace

namespace vne::interaction {

using namespace vne;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct Ortho2DController::Impl {
    CameraRig rig;
    InputMapper mapper;
    std::shared_ptr<Ortho2DManipulator> ortho2d_behavior;

    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;

    CursorState cursor;
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

Ortho2DController::Ortho2DController()
    : impl_(std::make_unique<Impl>()) {
    impl_->ortho2d_behavior = std::make_shared<Ortho2DManipulator>();
    impl_->rig.addManipulator(impl_->ortho2d_behavior);

    // Capture raw Impl* so the callback stays valid across moves.
    impl_->mapper.setActionCallback([impl = impl_.get()](CameraActionType a, const CameraCommandPayload& p, double dt) {
        impl->rig.onAction(a, p, dt);
    });

    rebuildRules();
}

Ortho2DController::~Ortho2DController() = default;
Ortho2DController::Ortho2DController(Ortho2DController&&) noexcept = default;
Ortho2DController& Ortho2DController::operator=(Ortho2DController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void Ortho2DController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    impl_->camera = camera;
    if (!camera) {
        VNE_LOG_DEBUG << "Ortho2DController: camera detached (null camera)";
    }
    impl_->rig.setCamera(camera);
}

void Ortho2DController::onResize(float w, float h) noexcept {
    impl_->viewport_w = w;
    impl_->viewport_h = h;
    impl_->rig.onResize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void Ortho2DController::onEvent(const events::Event& event) noexcept {
    dispatchMouseEvents(impl_->mapper, impl_->cursor, event, 0.0);
}

void Ortho2DController::onUpdate(double dt) noexcept {
    impl_->rig.onUpdate(dt);
}

// ---------------------------------------------------------------------------
// DOF
// ---------------------------------------------------------------------------

void Ortho2DController::setRotationEnabled(bool enabled) noexcept {
    rotation_enabled_ = enabled;
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->setRotateEnabled(enabled);
    }
    rebuildRules();
}

void Ortho2DController::setPanEnabled(bool enabled) noexcept {
    pan_enabled_ = enabled;
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->setPanEnabled(enabled);
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
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->setPanInertiaEnabled(enabled);
    }
}

void Ortho2DController::setRotateSensitivity(float degrees_per_pixel) noexcept {
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->setRotationSensitivityDegreesPerPixel(degrees_per_pixel);
    }
}

void Ortho2DController::setZoomSensitivity(float multiplier) noexcept {
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->setZoomSpeed(multiplier);
    }
}

void Ortho2DController::setPanSensitivity(float damping) noexcept {
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->setPanDamping(damping);
    }
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Ortho2DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->ortho2d_behavior) {
        impl_->ortho2d_behavior->fitToAABB(mn, mx);
    }
}

void Ortho2DController::reset() noexcept {
    impl_->cursor = {};
    impl_->rig.resetState();
    impl_->mapper.resetState();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& Ortho2DController::inputMapper() noexcept {
    return impl_->mapper;
}
Ortho2DManipulator& Ortho2DController::ortho2DManipulator() noexcept {
    return *impl_->ortho2d_behavior;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void Ortho2DController::rebuildRules() noexcept {
    std::vector<InputRule> rules;

    if (pan_enabled_) {
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(pan_binding_.button),
            .modifier_mask = static_cast<int>(pan_binding_.modifier_mask),
            .on_press = CameraActionType::eBeginPan,
            .on_release = CameraActionType::eEndPan,
            .on_delta = CameraActionType::ePanDelta,
        });
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eMiddle),
            .on_press = CameraActionType::eBeginPan,
            .on_release = CameraActionType::eEndPan,
            .on_delta = CameraActionType::ePanDelta,
        });
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
        // RMB = in-plane rotate (eRotateDelta handled by Ortho2DManipulator)
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(rotate_binding_.button),
            .modifier_mask = static_cast<int>(rotate_binding_.modifier_mask),
            .on_press = CameraActionType::eBeginRotate,
            .on_release = CameraActionType::eEndRotate,
            .on_delta = CameraActionType::eRotateDelta,
        });
    }

    impl_->mapper.setRules(rules);
}

}  // namespace vne::interaction
