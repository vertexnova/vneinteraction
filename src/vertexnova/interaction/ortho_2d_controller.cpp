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

#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/touch_event.h"

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
    OrthoPanZoomBehavior* ortho_pan_zoom = nullptr;  // non-owning alias into rig

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

Ortho2DController::Ortho2DController()
    : impl_(std::make_unique<Impl>()) {
    auto behavior = std::make_shared<OrthoPanZoomBehavior>();
    impl_->ortho_pan_zoom = behavior.get();
    impl_->rig.addBehavior(std::move(behavior));

    impl_->mapper.setActionCallback(
        [this](CameraActionType a, const CameraCommandPayload& p, double dt) { impl_->rig.onAction(a, p, dt); });

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
    if (!camera) { VNE_LOG_WARN << "Ortho2DController: setCamera called with null camera"; }
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
    constexpr double kDt = 0.0;

    switch (event.type()) {
        case events::EventType::eMouseMoved: {
            const auto& e = static_cast<const events::MouseMovedEvent&>(event);
            const float x = static_cast<float>(e.x());
            const float y = static_cast<float>(e.y());
            const float dx = impl_->first_mouse ? 0.0f : static_cast<float>(e.x() - impl_->last_x);
            const float dy = impl_->first_mouse ? 0.0f : static_cast<float>(e.y() - impl_->last_y);
            impl_->last_x = e.x();
            impl_->last_y = e.y();
            impl_->first_mouse = false;
            impl_->mapper.onMouseMove(x, y, dx, dy, kDt);
            break;
        }
        case events::EventType::eMouseButtonPressed: {
            const auto& e = static_cast<const events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                impl_->last_x = e.x();
                impl_->last_y = e.y();
            }
            impl_->first_mouse = false;
            impl_->mapper.onMouseButton(static_cast<int>(e.button()),
                                        true,
                                        static_cast<float>(impl_->last_x),
                                        static_cast<float>(impl_->last_y),
                                        kDt);
            break;
        }
        case events::EventType::eMouseButtonReleased: {
            const auto& e = static_cast<const events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                impl_->last_x = e.x();
                impl_->last_y = e.y();
            }
            impl_->mapper.onMouseButton(static_cast<int>(e.button()),
                                        false,
                                        static_cast<float>(impl_->last_x),
                                        static_cast<float>(impl_->last_y),
                                        kDt);
            break;
        }
        case events::EventType::eMouseScrolled: {
            const auto& e = static_cast<const events::MouseScrolledEvent&>(event);
            impl_->mapper.onMouseScroll(static_cast<float>(e.xOffset()),
                                        static_cast<float>(e.yOffset()),
                                        static_cast<float>(impl_->last_x),
                                        static_cast<float>(impl_->last_y),
                                        kDt);
            break;
        }
        case events::EventType::eTouchPress: {
            const auto& e = static_cast<const events::TouchPressEvent&>(event);
            impl_->last_x = e.x();
            impl_->last_y = e.y();
            impl_->first_mouse = false;
            break;
        }
        case events::EventType::eTouchMove: {
            const auto& e = static_cast<const events::TouchMoveEvent&>(event);
            const float dx = impl_->first_mouse ? 0.0f : static_cast<float>(e.x() - impl_->last_x);
            const float dy = impl_->first_mouse ? 0.0f : static_cast<float>(e.y() - impl_->last_y);
            impl_->last_x = e.x();
            impl_->last_y = e.y();
            impl_->first_mouse = false;
            impl_->mapper.onTouchPan(TouchPan{dx, dy}, kDt);
            break;
        }
        case events::EventType::eTouchRelease:
            impl_->first_mouse = true;
            break;
        default:
            break;
    }
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
    impl_->first_mouse = true;
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
