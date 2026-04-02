/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_rig.h"

#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"
#include "vertexnova/interaction/follow_manipulator.h"

#include <vertexnova/logging/logging.h>

#include <algorithm>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.camera_rig");
}  // namespace

namespace vne::interaction {

void CameraRig::addManipulator(std::shared_ptr<ICameraManipulator> manipulator) {
    if (!manipulator) {
        VNE_LOG_WARN << "CameraRig: addManipulator called with null manipulator, ignoring";
        return;
    }
    manipulators_.push_back(std::move(manipulator));
}

void CameraRig::removeManipulator(const std::shared_ptr<ICameraManipulator>& manipulator) {
    manipulators_.erase(std::remove(manipulators_.begin(), manipulators_.end(), manipulator), manipulators_.end());
}

void CameraRig::clearManipulators() {
    manipulators_.clear();
}

void CameraRig::onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept {
    for (auto& m : manipulators_) {
        if (m && m->isEnabled()) {
            m->onAction(action, payload, delta_time);
        }
    }
}

void CameraRig::onUpdate(double delta_time) noexcept {
    for (auto& m : manipulators_) {
        if (m && m->isEnabled()) {
            m->onUpdate(delta_time);
        }
    }
}

void CameraRig::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    for (auto& m : manipulators_) {
        if (m) {
            m->setCamera(camera);
        }
    }
}

void CameraRig::onResize(float width_px, float height_px) noexcept {
    for (auto& m : manipulators_) {
        if (m) {
            m->onResize(width_px, height_px);
        }
    }
}

void CameraRig::resetState() noexcept {
    for (auto& m : manipulators_) {
        if (m) {
            m->resetState();
        }
    }
}

// ---------------------------------------------------------------------------
// Factory methods
// ---------------------------------------------------------------------------

CameraRig CameraRig::makeOrbit() {
    CameraRig rig;
    rig.addManipulator(std::make_shared<OrbitalCameraManipulator>());
    return rig;
}

CameraRig CameraRig::makeTrackball() {
    CameraRig rig;
    auto m = std::make_shared<OrbitalCameraManipulator>();
    m->setRotationMode(OrbitRotationMode::eTrackball);
    rig.addManipulator(std::move(m));
    return rig;
}

CameraRig CameraRig::makeFps() {
    CameraRig rig;
    auto m = std::make_shared<FreeLookManipulator>();
    m->setConstrainWorldUp(true);
    rig.addManipulator(std::move(m));
    return rig;
}

CameraRig CameraRig::makeFly() {
    CameraRig rig;
    auto m = std::make_shared<FreeLookManipulator>();
    m->setConstrainWorldUp(false);
    rig.addManipulator(std::move(m));
    return rig;
}

CameraRig CameraRig::makeOrtho2D() {
    CameraRig rig;
    rig.addManipulator(std::make_shared<Ortho2DManipulator>());
    return rig;
}

CameraRig CameraRig::makeFollow() {
    CameraRig rig;
    rig.addManipulator(std::make_shared<FollowManipulator>());
    return rig;
}

CameraRig CameraRig::make2D() {
    return makeOrtho2D();
}

}  // namespace vne::interaction
