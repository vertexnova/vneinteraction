/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/inspect_3d_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"

#include "controller_event_dispatch.h"

#include <vertexnova/logging/logging.h>

#include <algorithm>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.inspect_3d");
}  // namespace

namespace vne::interaction {

using namespace vne;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct Inspect3DController::Impl {
    CameraRig rig;
    InputMapper mapper;
    std::shared_ptr<OrbitalCameraManipulator> orbit;  // shared ownership; also in rig

    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;

    OrbitRotationMode rotation_mode = OrbitRotationMode::eOrbit;
    bool rotation_enabled = true;
    bool pivot_on_double_click_enabled = true;
    bool pan_enabled = true;
    bool zoom_enabled = true;

    CursorState cursor;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<InputRule> buildInspectRules(bool rotation, bool pivot_on_double_click, bool pan, bool zoom) {
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
    }

    if (pivot_on_double_click) {
        // Double-click LMB = eSetPivotAtCursor (COI along view direction; independent of LMB rotate-drag)
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

Inspect3DController::Inspect3DController()
    : impl_(std::make_unique<Impl>()) {
    impl_->orbit = std::make_shared<OrbitalCameraManipulator>();
    impl_->orbit->setRotationMode(OrbitRotationMode::eOrbit);
    impl_->rig.addManipulator(impl_->orbit);

    // Wire mapper ->rig. Capture raw Impl* so the callback stays valid across moves.
    impl_->mapper.setActionCallback([impl = impl_.get()](CameraActionType a, const CameraCommandPayload& p, double dt) {
        impl->rig.onAction(a, p, dt);
    });

    rebuildRules();
}

Inspect3DController::~Inspect3DController() = default;
Inspect3DController::Inspect3DController(Inspect3DController&&) noexcept = default;
Inspect3DController& Inspect3DController::operator=(Inspect3DController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void Inspect3DController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    impl_->camera = camera;
    if (!camera) {
        VNE_LOG_DEBUG << "Inspect3DController: camera detached (null camera)";
    }
    impl_->rig.setCamera(camera);
}

void Inspect3DController::onResize(float w, float h) noexcept {
    impl_->viewport_w = w;
    impl_->viewport_h = h;
    impl_->rig.onResize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void Inspect3DController::onEvent(const events::Event& event, double delta_time) noexcept {
    dispatchMouseEvents(impl_->mapper, impl_->cursor, event, delta_time);
}

void Inspect3DController::onUpdate(double dt) noexcept {
    impl_->rig.onUpdate(dt);
}

// ---------------------------------------------------------------------------
// Rotation mode
// ---------------------------------------------------------------------------

void Inspect3DController::setRotationMode(OrbitRotationMode mode) noexcept {
    impl_->rotation_mode = mode;
    if (impl_->orbit) {
        impl_->orbit->setRotationMode(mode);
    }
}

OrbitRotationMode Inspect3DController::getRotationMode() const noexcept {
    return impl_->rotation_mode;
}

// ---------------------------------------------------------------------------
// Pivot
// ---------------------------------------------------------------------------

void Inspect3DController::setPivot(const vne::math::Vec3f& world_pos) noexcept {
    if (impl_->orbit) {
        impl_->orbit->setPivot(world_pos, CenterOfInterestSpace::eWorldSpace);
        impl_->orbit->setPivotMode(OrbitPivotMode::eFixed);
    }
}

void Inspect3DController::setPivotMode(OrbitPivotMode mode) noexcept {
    if (impl_->orbit) {
        impl_->orbit->setPivotMode(mode);
    }
}

OrbitPivotMode Inspect3DController::getPivotMode() const noexcept {
    if (impl_->orbit)
        return impl_->orbit->getPivotMode();
    return OrbitPivotMode::eCoi;
}

// ---------------------------------------------------------------------------
// DOF enable/disable
// ---------------------------------------------------------------------------

void Inspect3DController::setRotationEnabled(bool enabled) noexcept {
    impl_->rotation_enabled = enabled;
    rebuildRules();
}

bool Inspect3DController::isRotationEnabled() const noexcept {
    return impl_->rotation_enabled;
}

void Inspect3DController::setPivotOnDoubleClickEnabled(bool enabled) noexcept {
    impl_->pivot_on_double_click_enabled = enabled;
    rebuildRules();
}

bool Inspect3DController::isPivotOnDoubleClickEnabled() const noexcept {
    return impl_->pivot_on_double_click_enabled;
}

void Inspect3DController::setPanEnabled(bool enabled) noexcept {
    impl_->pan_enabled = enabled;
    rebuildRules();
}

void Inspect3DController::setZoomEnabled(bool enabled) noexcept {
    impl_->zoom_enabled = enabled;
    rebuildRules();
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Inspect3DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->orbit)
        impl_->orbit->fitToAABB(mn, mx);
}

void Inspect3DController::reset() noexcept {
    impl_->cursor = {};
    impl_->rig.resetState();
    impl_->mapper.resetState();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& Inspect3DController::inputMapper() noexcept {
    return impl_->mapper;
}

OrbitalCameraManipulator& Inspect3DController::orbitalCameraManipulator() noexcept {
    return *impl_->orbit;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void Inspect3DController::rebuildRules() noexcept {
    auto rules = buildInspectRules(impl_->rotation_enabled,
                                   impl_->pivot_on_double_click_enabled,
                                   impl_->pan_enabled,
                                   impl_->zoom_enabled);
    impl_->mapper.setRules(rules);
}

}  // namespace vne::interaction
