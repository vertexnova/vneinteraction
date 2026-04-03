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
#include "vertexnova/interaction/follow_manipulator.h"

#include "camera_controller_impl.h"

#include <algorithm>
#include <cmath>

namespace vne::interaction {

using namespace vne;

namespace {
constexpr float kDefaultFollowOffsetY = 2.0f;
constexpr float kDefaultFollowOffsetZ = 5.0f;
constexpr float kLagToDampingScale = 20.0f;
}  // namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

class FollowController::Impl {
    friend class FollowController;

   private:
    CameraControllerContext core_;
    std::shared_ptr<FollowManipulator> follow_;

    FollowController::TargetCallback target_cb_;
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

FollowController::FollowController()
    : impl_(std::make_unique<Impl>()) {
    impl_->follow_ = std::make_shared<FollowManipulator>();
    impl_->core_.rig.addManipulator(impl_->follow_);

    // Scroll = zoom (optional, user may want to adjust distance)
    impl_->core_.mapper.addRule({
        .trigger = InputRule::Trigger::eScroll,
        .on_delta = CameraActionType::eZoomAtCursor,
    });

    // Capture raw Impl* so the callback stays valid across moves.
    impl_->core_.mapper.setActionCallback([impl = impl_.get()](CameraActionType a,
                                                               const CameraCommandPayload& p,
                                                               double dt) { impl->core_.rig.onAction(a, p, dt); });
}

FollowController::~FollowController() = default;
FollowController::FollowController(FollowController&&) noexcept = default;
FollowController& FollowController::operator=(FollowController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void FollowController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    impl_->core_.setCamera(std::move(camera));
}

void FollowController::onResize(float w, float h) noexcept {
    impl_->core_.onResize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void FollowController::onUpdate(double dt) noexcept {
    // Poll dynamic target each frame before updating rig
    if (impl_->target_cb_ && impl_->follow_) {
        const vne::math::Mat4f t = impl_->target_cb_();
        // Extract translation column
        impl_->follow_->setTargetWorld({t[3][0], t[3][1], t[3][2]});
    }
    impl_->core_.onUpdate(dt);
}

void FollowController::onEvent(const events::Event& event, double delta_time) noexcept {
    dispatchMouseEvents(impl_->core_.mapper, impl_->core_.cursor, event, delta_time);
}

// ---------------------------------------------------------------------------
// Target
// ---------------------------------------------------------------------------

void FollowController::setTarget(TargetCallback cb) noexcept {
    impl_->target_cb_ = std::move(cb);
}

void FollowController::setTarget(const vne::math::Mat4f& world_transform) noexcept {
    impl_->target_cb_ = nullptr;
    if (impl_->follow_) {
        impl_->follow_->setTargetWorld({world_transform[3][0], world_transform[3][1], world_transform[3][2]});
    }
}

// ---------------------------------------------------------------------------
// Follow parameters
// ---------------------------------------------------------------------------

void FollowController::setOffset(const vne::math::Vec3f& offset) noexcept {
    if (impl_->follow_) {
        impl_->follow_->setOffset(offset);
    }
}

vne::math::Vec3f FollowController::getOffset() const noexcept {
    return impl_->follow_ ? impl_->follow_->getOffset()
                          : vne::math::Vec3f{0.0f, kDefaultFollowOffsetY, kDefaultFollowOffsetZ};
}

void FollowController::setLag(float lag) noexcept {
    // Convert lag [0,1] to damping: lag=0 ->very high damping (instant), lag=1 ->0 (never arrives)
    // damping = -ln(lag_remainder) / typical_dt; we use a simple inversion: damping = (1-lag)*20
    if (impl_->follow_) {
        const float damping = (1.0f - std::clamp(lag, 0.0f, 0.999f)) * kLagToDampingScale;
        impl_->follow_->setDamping(damping);
    }
}

float FollowController::getLag() const noexcept {
    if (!impl_->follow_) {
        return 0.0f;
    }
    // Invert the formula above
    return 1.0f - (impl_->follow_->getDamping() / kLagToDampingScale);
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void FollowController::reset() noexcept {
    impl_->core_.resetRigAndInteraction();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& FollowController::inputMapper() noexcept {
    return impl_->core_.mapper;
}
FollowManipulator& FollowController::followManipulator() noexcept {
    return *impl_->follow_;
}

}  // namespace vne::interaction
