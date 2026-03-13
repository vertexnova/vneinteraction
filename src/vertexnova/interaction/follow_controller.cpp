/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/follow_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/track_behavior.h"

#include "vertexnova/events/mouse_event.h"

#include <cmath>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct FollowController::Impl {
    CameraRig rig;
    InputMapper mapper;
    TrackBehavior* track = nullptr;  // non-owning alias into rig

    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;

    double last_x = 0.0;
    double last_y = 0.0;

    FollowController::TargetCallback target_cb;
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

FollowController::FollowController()
    : impl_(std::make_unique<Impl>()) {
    auto behavior = std::make_shared<TrackBehavior>();
    impl_->track = behavior.get();
    impl_->rig.addBehavior(std::move(behavior));

    // Scroll = zoom (optional, user may want to adjust distance)
    impl_->mapper.addRule({
        .trigger = InputRule::Trigger::eScroll,
        .on_delta = CameraActionType::eZoomAtCursor,
    });

    impl_->mapper.setActionCallback(
        [this](CameraActionType a, const CameraCommandPayload& p, double dt) { impl_->rig.onAction(a, p, dt); });
}

FollowController::~FollowController() = default;
FollowController::FollowController(FollowController&&) noexcept = default;
FollowController& FollowController::operator=(FollowController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void FollowController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    impl_->camera = camera;
    impl_->rig.setCamera(camera);
}

void FollowController::setViewportSize(float w, float h) noexcept {
    impl_->viewport_w = w;
    impl_->viewport_h = h;
    impl_->rig.setViewportSize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void FollowController::onUpdate(double dt) noexcept {
    // Poll dynamic target each frame before updating rig
    if (impl_->target_cb && impl_->track) {
        const vne::math::Mat4f t = impl_->target_cb();
        // Extract translation column
        impl_->track->setTargetWorld({t[3][0], t[3][1], t[3][2]});
    }
    impl_->rig.onUpdate(dt);
}

void FollowController::onEvent(const vne::events::Event& event) noexcept {
    using ET = vne::events::EventType;
    constexpr double kDt = 0.0;

    if (event.type() == ET::eMouseScrolled) {
        const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
        impl_->mapper.onMouseScroll(static_cast<float>(e.xOffset()),
                                    static_cast<float>(e.yOffset()),
                                    static_cast<float>(impl_->last_x),
                                    static_cast<float>(impl_->last_y),
                                    kDt);
    }
}

// ---------------------------------------------------------------------------
// Target
// ---------------------------------------------------------------------------

void FollowController::setTarget(TargetCallback cb) noexcept {
    impl_->target_cb = std::move(cb);
}

void FollowController::setTarget(const vne::math::Mat4f& world_transform) noexcept {
    impl_->target_cb = nullptr;
    if (impl_->track) {
        impl_->track->setTargetWorld({world_transform[3][0], world_transform[3][1], world_transform[3][2]});
    }
}

// ---------------------------------------------------------------------------
// Follow parameters
// ---------------------------------------------------------------------------

void FollowController::setOffset(const vne::math::Vec3f& offset) noexcept {
    if (impl_->track)
        impl_->track->setOffset(offset);
}

vne::math::Vec3f FollowController::getOffset() const noexcept {
    return impl_->track ? impl_->track->getOffset() : vne::math::Vec3f{0.0f, 2.0f, 5.0f};
}

void FollowController::setLag(float lag) noexcept {
    // Convert lag [0,1] to damping: lag=0 ->very high damping (instant), lag=1 ->0 (never arrives)
    // damping = -ln(lag_remainder) / typical_dt; we use a simple inversion: damping = (1-lag)*20
    if (impl_->track) {
        const float damping = (1.0f - std::clamp(lag, 0.0f, 0.999f)) * 20.0f;
        impl_->track->setDamping(damping);
    }
}

float FollowController::getLag() const noexcept {
    if (!impl_->track)
        return 0.0f;
    // Invert the formula above
    return 1.0f - (impl_->track->getDamping() / 20.0f);
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void FollowController::reset() noexcept {
    impl_->rig.resetState();
    impl_->mapper.resetState();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& FollowController::inputMapper() noexcept {
    return impl_->mapper;
}
TrackBehavior& FollowController::trackBehavior() noexcept {
    return *impl_->track;
}

}  // namespace vne::interaction
