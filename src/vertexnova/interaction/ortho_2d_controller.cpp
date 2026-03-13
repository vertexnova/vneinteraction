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
#include "vertexnova/interaction/ortho_pan_zoom_behavior.h"

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
    std::shared_ptr<OrthoPanZoomBehavior> ortho_pan_zoom;

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
    impl_->ortho_pan_zoom = std::make_shared<OrthoPanZoomBehavior>();
    impl_->rig.addBehavior(impl_->ortho_pan_zoom);

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

void Ortho2DController::setViewportSize(float w, float h) noexcept {
    impl_->viewport_w = w;
    impl_->viewport_h = h;
    impl_->rig.setViewportSize(w, h);
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
    rebuildRules();
}

void Ortho2DController::setPanEnabled(bool enabled) noexcept {
    pan_enabled_ = enabled;
    rebuildRules();
}

void Ortho2DController::setZoomEnabled(bool enabled) noexcept {
    zoom_enabled_ = enabled;
    rebuildRules();
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Ortho2DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->ortho_pan_zoom)
        impl_->ortho_pan_zoom->fitToAABB(mn, mx);
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
OrthoPanZoomBehavior& Ortho2DController::orthoPanZoomBehavior() noexcept {
    return *impl_->ortho_pan_zoom;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void Ortho2DController::rebuildRules() noexcept {
    std::vector<InputRule> rules;

    if (pan_enabled_) {
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eLeft),
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
            .on_delta = CameraActionType::eZoomAtCursor,
        });
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPinch,
            .on_delta = CameraActionType::eZoomAtCursor,
        });
    }

    if (rotation_enabled_) {
        // RMB = in-plane rotate (eRotateDelta — OrthoPanZoomBehavior ignores it,
        // but a future OrbitArcballBehavior layer could handle it)
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eRight),
            .on_press = CameraActionType::eBeginRotate,
            .on_release = CameraActionType::eEndRotate,
            .on_delta = CameraActionType::eRotateDelta,
        });
    }

    impl_->mapper.setRules(rules);
}

}  // namespace vne::interaction
