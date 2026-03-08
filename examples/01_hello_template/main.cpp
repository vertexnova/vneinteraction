/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "common/logging_guard.h"
#include "vertexnova/interaction/version.h"
#include "vertexnova/interaction/interaction.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    VNE_LOG_INFO << "VneInteraction version: " << vne::interaction::get_version();

    auto factory = std::make_shared<vne::interaction::CameraManipulatorFactory>();
    auto orbit = factory->create(vne::interaction::CameraManipulatorType::eOrbit);
    if (orbit) {
        orbit->setViewportSize(1280.0f, 720.0f);
        VNE_LOG_INFO << "OrbitManipulator created, scene scale: " << orbit->getSceneScale();
    }

    return 0;
}
