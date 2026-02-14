/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/interaction.h"
#include "vertexnova/scene/scene.h"
#include <cassert>
#include <memory>

int main() {
    using namespace vne::interaction;
    using namespace vne::scene;
    using namespace vne::math;

    auto cam = CameraFactory::createPerspective(
        PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 100.0f));
    assert(cam);

    auto controller = std::make_unique<CameraSystemController>(CameraManipulatorType::eOrbitArcball);
    controller->setCamera(cam);
    controller->setViewportSize(1920.0f, 1080.0f);
    assert(controller->getCamera() == cam);
    assert(controller->getManipulatorType() == CameraManipulatorType::eOrbitArcball);

    auto manip = CameraManipulatorFactory::create(CameraManipulatorType::eFpsFly);
    assert(manip);
    assert(manip->supportsPerspective());

    controller->setManipulator(CameraManipulatorType::eOrthoPanZoom);
    assert(controller->getManipulatorType() == CameraManipulatorType::eOrthoPanZoom);

    TouchPan pan{10.0f, -5.0f};
    TouchPinch pinch{1.2f, 100.0f, 100.0f};
    controller->handleTouchPan(pan, 0.016);
    controller->handleTouchPinch(pinch, 0.016);
    controller->update(0.016);

    return 0;
}
