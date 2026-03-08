/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"

namespace vne::interaction {

std::shared_ptr<ICameraManipulator> CameraManipulatorFactory::create(CameraManipulatorType type) const {
    switch (type) {
        case CameraManipulatorType::eOrbit:
            return std::make_shared<OrbitManipulator>();
        case CameraManipulatorType::eArcball:
            return std::make_shared<ArcballManipulator>();
        case CameraManipulatorType::eFps:
            return std::make_shared<FpsManipulator>();
        case CameraManipulatorType::eFly:
            return std::make_shared<FlyManipulator>();
        case CameraManipulatorType::eOrthoPanZoom:
            return std::make_shared<OrthoPanZoomManipulator>();
        case CameraManipulatorType::eFollow:
            return std::make_shared<FollowManipulator>();
    }
    return nullptr;
}

}  // namespace vne::interaction
