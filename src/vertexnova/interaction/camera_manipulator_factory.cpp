/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/fps_fly_manipulator.h"
#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/interaction/orbit_arcball_manipulator.h"
#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"

namespace vne::interaction {

std::unique_ptr<ICameraManipulator> CameraManipulatorFactory::create(CameraManipulatorType type) noexcept {
    switch (type) {
        case CameraManipulatorType::eOrbitArcball:
            return std::make_unique<OrbitArcballManipulator>();
        case CameraManipulatorType::eFpsFly:
            return std::make_unique<FpsFlyManipulator>();
        case CameraManipulatorType::eOrthoPanZoom:
            return std::make_unique<OrthoPanZoomManipulator>();
        case CameraManipulatorType::eFollow:
            return std::make_unique<FollowManipulator>();
        default:
            return std::make_unique<OrbitArcballManipulator>();
    }
}

}  // namespace vne::interaction
