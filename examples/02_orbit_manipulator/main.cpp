/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 02 - Orbit Manipulator
 * Demonstrates: creating an OrbitManipulator, configuring its properties,
 * simulating a mouse-drag orbit sequence, applying inertia via update(),
 * and using fitToAABB() to frame a bounding box.
 *
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/input_simulation.h"
#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/orbit_manipulator.h"

#include <vertexnova/math/core/core.h>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;
    using namespace vne::interaction::examples;

    // --- Create via factory (returns ICameraManipulator) ---
    const vne::interaction::CameraManipulatorFactory factory;
    auto manipulator = factory.create(vne::interaction::CameraManipulatorType::eOrbit);

    // Downcast to access orbit-specific API
    auto* orbit = dynamic_cast<vne::interaction::OrbitManipulator*>(manipulator.get());

    // --- Configure ---
    manipulator->setViewportSize(1280.0f, 720.0f);
    orbit->setOrbitDistance(10.0f);
    orbit->setRotationSpeed(0.3f);
    orbit->setPanSpeed(1.0f);
    orbit->setZoomSpeed(1.15f);
    orbit->setRotationDamping(8.0f);
    orbit->setPanDamping(10.0f);
    orbit->setZoomMethod(vne::interaction::ZoomMethod::eDollyToCoi);

    VNE_LOG_INFO << "OrbitManipulator created";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance() << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    constexpr double dt = 1.0 / 60.0;

    // --- Simulate: horizontal orbit drag (30 frames × 2 px/frame = 60 px total) ---
    simulateMouseDrag(*manipulator,
                      vne::interaction::MouseButton::eLeft,
                      640.0f,
                      360.0f,
                      /*total_dx=*/60.0f,
                      /*total_dy=*/0.0f,
                      30,
                      dt);

    // --- Let inertia decay over 30 frames ---
    runFrames(*manipulator, 30, dt);

    VNE_LOG_INFO << "After orbit + inertia decay:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: 2× scroll zoom in ---
    simulateMouseScroll(*manipulator, -1.0f, 640.0f, 360.0f, 2, dt);
    VNE_LOG_INFO << "After 2x scroll zoom in:";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance();

    // --- Fit to a unit AABB ---
    const vne::math::Vec3f aabb_min{-1.0f, -1.0f, -1.0f};
    const vne::math::Vec3f aabb_max{1.0f, 1.0f, 1.0f};
    manipulator->fitToAABB(aabb_min, aabb_max);
    VNE_LOG_INFO << "After fitToAABB([-1,-1,-1] to [1,1,1]):";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance() << "  sceneScale=" << manipulator->getSceneScale();

    // --- Reset ---
    manipulator->resetState();
    VNE_LOG_INFO << "After resetState:";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance();

    return 0;
}
