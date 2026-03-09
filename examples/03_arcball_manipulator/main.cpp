/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 03 - Arcball Manipulator
 * Demonstrates: creating an ArcballManipulator, configuring its properties,
 * simulating a virtual-trackball drag, inertia decay, and pan.
 *
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/input_simulation.h"
#include "common/logging_guard.h"
#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;
    using namespace vne::interaction::examples;

    // --- Create via factory ---
    const vne::interaction::CameraManipulatorFactory factory;
    auto manipulator = factory.create(vne::interaction::CameraManipulatorType::eArcball);

    auto* arcball = dynamic_cast<vne::interaction::ArcballManipulator*>(manipulator.get());

    // --- Configure ---
    manipulator->setViewportSize(1280.0f, 720.0f);
    arcball->setOrbitDistance(8.0f);
    arcball->setRotationSpeed(1.0f);
    arcball->setPanSpeed(1.0f);
    arcball->setZoomSpeed(1.1f);
    arcball->setRotationDamping(8.0f);
    arcball->setPanDamping(10.0f);

    VNE_LOG_INFO << "ArcballManipulator created";
    VNE_LOG_INFO << "  orbitDistance=" << arcball->getOrbitDistance()
                 << "  sceneScale=" << manipulator->getSceneScale();

    constexpr double dt = 1.0 / 60.0;

    // --- Simulate: arcball drag (20 frames × dx=3, dy=1.5) ---
    simulateMouseDrag(*manipulator,
                      vne::interaction::MouseButton::eLeft,
                      400.0f,
                      300.0f,
                      /*total_dx=*/60.0f,
                      /*total_dy=*/30.0f,
                      20,
                      dt);
    runFrames(*manipulator, 30, dt);
    VNE_LOG_INFO << "After arcball drag + inertia:";
    VNE_LOG_INFO << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: right-mouse pan (15 frames × dx=4) ---
    simulateMouseDrag(*manipulator,
                      vne::interaction::MouseButton::eRight,
                      640.0f,
                      360.0f,
                      /*total_dx=*/60.0f,
                      /*total_dy=*/0.0f,
                      15,
                      dt);
    runFrames(*manipulator, 30, dt);
    VNE_LOG_INFO << "After pan + inertia decay:";
    VNE_LOG_INFO << "  coi=" << arcball->getCenterOfInterestWorld().x() << ", "
                 << arcball->getCenterOfInterestWorld().y() << ", " << arcball->getCenterOfInterestWorld().z();

    // --- fitToAABB ---
    manipulator->fitToAABB({-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f});
    VNE_LOG_INFO << "After fitToAABB([-2,-2,-2] to [2,2,2]):";
    VNE_LOG_INFO << "  orbitDistance=" << arcball->getOrbitDistance()
                 << "  sceneScale=" << manipulator->getSceneScale();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
