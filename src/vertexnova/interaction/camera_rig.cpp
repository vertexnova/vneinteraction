/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_rig.h"

#include "vertexnova/interaction/orbital_camera_behavior.h"
#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/interaction/ortho_2d_behavior.h"
#include "vertexnova/interaction/follow_behavior.h"

#include <vertexnova/logging/logging.h>

#include <algorithm>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.camera_rig");
}  // namespace

namespace vne::interaction {

void CameraRig::addBehavior(std::shared_ptr<ICameraBehavior> behavior) {
    if (!behavior) {
        VNE_LOG_WARN << "CameraRig: addBehavior called with null behavior, ignoring";
        return;
    }
    behaviors_.push_back(std::move(behavior));
}

void CameraRig::removeBehavior(const std::shared_ptr<ICameraBehavior>& behavior) {
    behaviors_.erase(std::remove(behaviors_.begin(), behaviors_.end(), behavior), behaviors_.end());
}

void CameraRig::clearBehaviors() {
    behaviors_.clear();
}

void CameraRig::onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept {
    for (auto& b : behaviors_) {
        if (b && b->isEnabled()) {
            b->onAction(action, payload, delta_time);
        }
    }
}

void CameraRig::onUpdate(double delta_time) noexcept {
    for (auto& b : behaviors_) {
        if (b && b->isEnabled()) {
            b->onUpdate(delta_time);
        }
    }
}

void CameraRig::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    for (auto& b : behaviors_) {
        if (b) {
            b->setCamera(camera);
        }
    }
}

void CameraRig::onResize(float width_px, float height_px) noexcept {
    for (auto& b : behaviors_) {
        if (b) {
            b->onResize(width_px, height_px);
        }
    }
}

void CameraRig::resetState() noexcept {
    for (auto& b : behaviors_) {
        if (b) {
            b->resetState();
        }
    }
}

// ---------------------------------------------------------------------------
// Factory methods
// ---------------------------------------------------------------------------

CameraRig CameraRig::makeOrbit() {
    CameraRig rig;
    rig.addBehavior(std::make_shared<OrbitalCameraBehavior>());
    return rig;
}

CameraRig CameraRig::makeTrackball() {
    CameraRig rig;
    auto b = std::make_shared<OrbitalCameraBehavior>();
    b->setRotationMode(OrbitRotationMode::eTrackball);
    rig.addBehavior(std::move(b));
    return rig;
}

CameraRig CameraRig::makeFps() {
    CameraRig rig;
    auto b = std::make_shared<FreeLookBehavior>();
    b->setConstrainWorldUp(true);
    rig.addBehavior(std::move(b));
    return rig;
}

CameraRig CameraRig::makeFly() {
    CameraRig rig;
    auto b = std::make_shared<FreeLookBehavior>();
    b->setConstrainWorldUp(false);
    rig.addBehavior(std::move(b));
    return rig;
}

CameraRig CameraRig::makeOrtho2D() {
    CameraRig rig;
    rig.addBehavior(std::make_shared<Ortho2DBehavior>());
    return rig;
}

CameraRig CameraRig::makeFollow() {
    CameraRig rig;
    rig.addBehavior(std::make_shared<FollowBehavior>());
    return rig;
}

CameraRig CameraRig::make2D() {
    return makeOrtho2D();
}

}  // namespace vne::interaction
