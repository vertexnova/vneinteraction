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
#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/interaction/orbit_trackball_behavior.h"

#include "controller_event_dispatch.h"
#include "vertexnova/events/key_event.h"

#include <vertexnova/logging/logging.h>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.navigation_3d");
}  // namespace

namespace vne::interaction {

using namespace vne;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct Navigation3DController::Impl {
    CameraRig rig;
    InputMapper mapper;
    std::shared_ptr<FreeLookBehavior> free_look;  // shared ownership; also in rig
    std::shared_ptr<OrbitTrackballBehavior> orbit;  // shared ownership; only in eGame mode

    NavigateMode mode = NavigateMode::eFps;

    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;

    CursorState cursor;
};

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
    impl_->camera = camera;
    if (!camera) {
        VNE_LOG_DEBUG << "Navigation3DController: camera detached (null camera)";
    }
    impl_->rig.setCamera(camera);
}

void Navigation3DController::onResize(float w, float h) noexcept {
    const float clamped_w = (w < 1.0f) ? 1.0f : w;
    const float clamped_h = (h < 1.0f) ? 1.0f : h;
    impl_->viewport_w = clamped_w;
    impl_->viewport_h = clamped_h;
    impl_->rig.onResize(clamped_w, clamped_h);
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
            impl_->mapper.onKey(static_cast<int>(e.keyCode()), true, delta_time);
            return;
        }
        case events::EventType::eKeyReleased: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->mapper.onKey(static_cast<int>(e.keyCode()), false, delta_time);
            return;
        }
        default:
            break;
    }
    dispatchMouseEvents(impl_->mapper, impl_->cursor, event, delta_time);
}

void Navigation3DController::onUpdate(double dt) noexcept {
    impl_->rig.onUpdate(dt);
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
    if (impl_->free_look)
        impl_->free_look->setMoveSpeed(s);
}

float Navigation3DController::getMoveSpeed() const noexcept {
    return impl_->free_look ? impl_->free_look->getMoveSpeed() : 3.0f;
}

void Navigation3DController::setMouseSensitivity(float s) noexcept {
    if (impl_->free_look)
        impl_->free_look->setMouseSensitivity(s);
}

float Navigation3DController::getMouseSensitivity() const noexcept {
    return impl_->free_look ? impl_->free_look->getMouseSensitivity() : 0.15f;
}

void Navigation3DController::setSprintMultiplier(float m) noexcept {
    if (impl_->free_look)
        impl_->free_look->setSprintMultiplier(m);
}

float Navigation3DController::getSprintMultiplier() const noexcept {
    return impl_->free_look ? impl_->free_look->getSprintMultiplier() : 4.0f;
}

void Navigation3DController::setSlowMultiplier(float m) noexcept {
    if (impl_->free_look)
        impl_->free_look->setSlowMultiplier(m);
}

float Navigation3DController::getSlowMultiplier() const noexcept {
    return impl_->free_look ? impl_->free_look->getSlowMultiplier() : 0.2f;
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Navigation3DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->free_look)
        impl_->free_look->fitToAABB(mn, mx);
}

void Navigation3DController::reset() noexcept {
    impl_->cursor = {};
    impl_->rig.resetState();
    impl_->mapper.resetState();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& Navigation3DController::inputMapper() noexcept {
    return impl_->mapper;
}
FreeLookBehavior& Navigation3DController::freeLookBehavior() noexcept {
    return *impl_->free_look;
}
OrbitTrackballBehavior* Navigation3DController::orbitTrackballBehavior() noexcept {
    return impl_->orbit.get();
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
            impl_->free_look->setHandleZoom(false);
            VNE_LOG_DEBUG << "Navigation3DController: Game mode — orbit owns zoom, free_look zoom disabled";
            impl_->orbit = std::make_shared<OrbitTrackballBehavior>();
            impl_->orbit->setRotationMode(OrbitRotationMode::eOrbit);
            impl_->rig.addBehavior(impl_->orbit);
            break;
        }
    }

    impl_->rig.addBehavior(impl_->free_look);

    // Re-attach camera and viewport
    if (impl_->camera)
        impl_->rig.setCamera(impl_->camera);
    impl_->rig.onResize(impl_->viewport_w, impl_->viewport_h);

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
    // Capture raw Impl* so the callback stays valid across moves.
    impl_->mapper.setActionCallback([impl = impl_.get()](CameraActionType a, const CameraCommandPayload& p, double dt) {
        impl->rig.onAction(a, p, dt);
    });
}

}  // namespace vne::interaction
