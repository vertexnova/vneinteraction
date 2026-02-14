/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/interaction.h"
#include "vertexnova/scene/scene.h"
#include <iostream>
#include <memory>

int main() {
    using namespace vne::interaction;
    using namespace vne::scene;
    using namespace vne::math;

    auto cam = CameraFactory::createPerspective(
        PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 100.0f));
    cam->setPosition(Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(Vec3f(0.0f, 0.0f, 0.0f));

    CameraSystemController controller(CameraManipulatorType::eOrbitArcball);
    controller.setCamera(cam);
    controller.setViewportSize(800.0f, 600.0f);
    controller.update(0.016);

    std::cout << "Camera position: " << cam->getPosition().x() << ", " << cam->getPosition().y() << ", "
              << cam->getPosition().z() << std::endl;
    std::cout << "Controller manipulator: OrbitArcball" << std::endl;

    return 0;
}
