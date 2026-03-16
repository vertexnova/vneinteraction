/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/inspect_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/orbit_arcball_behavior.h"

#include "controller_event_dispatch.h"

#include <vertexnova/logging/logging.h>

#include <algorithm>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.inspect");
}  // namespace

namespace vne::interaction {

using namespace vne;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct InspectController::Impl {
    CameraRig rig;
    InputMapper mapper;
    std::shared_ptr<OrbitArcballBehavior> orbit;  // shared ownership; also in rig

    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;

    OrbitRotationMode rotation_mode = OrbitRotationMode::eArcball;
    bool rotation_enabled = true;
    bool pan_enabled = true;
    bool zoom_enabled = true;

    CursorState cursor;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<InputRule> buildInspectRules(bool rotation, bool pan, bool zoom) {
    std::vector<InputRule> rules;

    if (rotation) {
        // LMB = rotate
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eLeft),
            .on_press = CameraActionType::eBeginRotate,
            .on_release = CameraActionType::eEndRotate,
            .on_delta = CameraActionType::eRotateDelta,
        });
        // Shift+LMB = pan alias
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eLeft),
            .modifier_mask = kModShift,
            .on_press = CameraActionType::eBeginPan,
            .on_release = CameraActionType::eEndPan,
            .on_delta = CameraActionType::ePanDelta,
        });
        // Double-click LMB = set pivot at cursor
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseDblClick,
            .code = static_cast<int>(MouseButton::eLeft),
            .on_press = CameraActionType::eSetPivotAtCursor,
        });
    }

    if (pan) {
        // RMB = pan
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eRight),
            .on_press = CameraActionType::eBeginPan,
            .on_release = CameraActionType::eEndPan,
            .on_delta = CameraActionType::ePanDelta,
        });
        // MMB = pan
        rules.push_back({
            .trigger = InputRule::Trigger::eMouseButton,
            .code = static_cast<int>(MouseButton::eMiddle),
            .on_press = CameraActionType::eBeginPan,
            .on_release = CameraActionType::eEndPan,
            .on_delta = CameraActionType::ePanDelta,
        });
        // Touch pan
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPan,
            .on_delta = CameraActionType::ePanDelta,
        });
    }

    if (zoom) {
        // Scroll = zoom
        rules.push_back({
            .trigger = InputRule::Trigger::eScroll,
            .on_delta = CameraActionType::eZoomAtCursor,
        });
        // Pinch = zoom
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPinch,
            .on_delta = CameraActionType::eZoomAtCursor,
        });
    }

    return rules;
}

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

InspectController::InspectController()
    : impl_(std::make_unique<Impl>()) {
    // Build rig with arcball orbit
    impl_->orbit = std::make_shared<OrbitArcballBehavior>();
    impl_->orbit->setRotationMode(OrbitRotationMode::eArcball);
    impl_->rig.addBehavior(impl_->orbit);

    // Wire mapper ->rig
    impl_->mapper.setActionCallback(
        [this](CameraActionType a, const CameraCommandPayload& p, double dt) { impl_->rig.onAction(a, p, dt); });

    rebuildRules();
}

InspectController::~InspectController() = default;
InspectController::InspectController(InspectController&&) noexcept = default;
InspectController& InspectController::operator=(InspectController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void InspectController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    impl_->camera = camera;
    if (!camera) {
        VNE_LOG_DEBUG << "InspectController: camera detached (null camera)";
    }
    impl_->rig.setCamera(camera);
}

void InspectController::onResize(float w, float h) noexcept {
    impl_->viewport_w = w;
    impl_->viewport_h = h;
    impl_->rig.onResize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void InspectController::onEvent(const events::Event& event, double delta_time) noexcept {
    dispatchMouseEvents(impl_->mapper, impl_->cursor, event, delta_time);
}

void InspectController::onUpdate(double dt) noexcept {
    impl_->rig.onUpdate(dt);
}

// ---------------------------------------------------------------------------
// Rotation mode
// ---------------------------------------------------------------------------

void InspectController::setRotationMode(OrbitRotationMode mode) noexcept {
    impl_->rotation_mode = mode;
    if (impl_->orbit) {
        impl_->orbit->setRotationMode(mode);
    }
}

OrbitRotationMode InspectController::getRotationMode() const noexcept {
    return impl_->rotation_mode;
}

// ---------------------------------------------------------------------------
// Pivot
// ---------------------------------------------------------------------------

void InspectController::setPivot(const vne::math::Vec3f& world_pos) noexcept {
    if (impl_->orbit) {
        impl_->orbit->setPivot(world_pos, CenterOfInterestSpace::eWorldSpace);
        impl_->orbit->setPivotMode(OrbitPivotMode::eFixed);
    }
}

void InspectController::setPivotMode(OrbitPivotMode mode) noexcept {
    if (impl_->orbit) {
        impl_->orbit->setPivotMode(mode);
    }
}

OrbitPivotMode InspectController::getPivotMode() const noexcept {
    if (impl_->orbit)
        return impl_->orbit->getPivotMode();
    return OrbitPivotMode::eCoi;
}

// ---------------------------------------------------------------------------
// DOF enable/disable
// ---------------------------------------------------------------------------

void InspectController::setRotationEnabled(bool enabled) noexcept {
    impl_->rotation_enabled = enabled;
    rebuildRules();
}

void InspectController::setPanEnabled(bool enabled) noexcept {
    impl_->pan_enabled = enabled;
    rebuildRules();
}

void InspectController::setZoomEnabled(bool enabled) noexcept {
    impl_->zoom_enabled = enabled;
    rebuildRules();
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void InspectController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->orbit)
        impl_->orbit->fitToAABB(mn, mx);
}

void InspectController::reset() noexcept {
    impl_->cursor = {};
    impl_->rig.resetState();
    impl_->mapper.resetState();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& InspectController::inputMapper() noexcept {
    return impl_->mapper;
}

OrbitArcballBehavior& InspectController::orbitArcballBehavior() noexcept {
    return *impl_->orbit;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void InspectController::rebuildRules() noexcept {
    auto rules = buildInspectRules(impl_->rotation_enabled, impl_->pan_enabled, impl_->zoom_enabled);
    impl_->mapper.setRules(rules);
}

}  // namespace vne::interaction
