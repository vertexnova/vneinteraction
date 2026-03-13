/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_rig.h"

#include "vertexnova/interaction/orbit_arcball_behavior.h"
#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/interaction/ortho_pan_zoom_behavior.h"
#include "vertexnova/interaction/follow_behavior.h"

#include <algorithm>

namespace vne::interaction {

void CameraRig::addBehavior(std::shared_ptr<ICameraBehavior> behavior) {
    if (behavior) {
        behaviors_.push_back(std::move(behavior));
    }
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

void CameraRig::setViewportSize(float width_px, float height_px) noexcept {
    for (auto& b : behaviors_) {
        if (b) {
            b->setViewportSize(width_px, height_px);
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
    rig.addBehavior(std::make_shared<OrbitArcballBehavior>());
    return rig;
}

CameraRig CameraRig::makeArcball() {
    CameraRig rig;
    auto b = std::make_shared<OrbitArcballBehavior>();
    b->setRotationMode(OrbitRotationMode::eArcball);
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

CameraRig CameraRig::makeOrthoPanZoom() {
    CameraRig rig;
    rig.addBehavior(std::make_shared<OrthoPanZoomBehavior>());
    return rig;
}

CameraRig CameraRig::makeFollow() {
    CameraRig rig;
    rig.addBehavior(std::make_shared<FollowBehavior>());
    return rig;
}

CameraRig CameraRig::makeGameCamera() {
    CameraRig rig;
    rig.addBehavior(std::make_shared<OrbitArcballBehavior>());
    auto fl = std::make_shared<FreeLookBehavior>();
    fl->setConstrainWorldUp(true);
    fl->setHandleZoom(false);  // orbit behavior owns eZoomAtCursor
    rig.addBehavior(std::move(fl));
    return rig;
}

CameraRig CameraRig::make2D() {
    return makeOrthoPanZoom();
}

}  // namespace vne::interaction
