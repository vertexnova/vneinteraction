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

#include "common/logging_guard.h"
#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

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

    // --- Simulate: arcball drag (left mouse) ---
    constexpr double dt = 1.0 / 60.0;
    manipulator->handleMouseButton(
        static_cast<int>(vne::interaction::MouseButton::eLeft), /*pressed=*/true,
        400.0f, 300.0f, dt);

    float mx = 400.0f;
    float my = 300.0f;
    for (int i = 0; i < 20; ++i) {
        const float dx = 3.0f;
        const float dy = 1.5f;
        manipulator->handleMouseMove(mx + dx, my + dy, dx, dy, dt);
        mx += dx;
        my += dy;
    }

    manipulator->handleMouseButton(
        static_cast<int>(vne::interaction::MouseButton::eLeft), /*pressed=*/false,
        mx, my, dt);

    // --- Inertia decay ---
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }
    VNE_LOG_INFO << "After arcball drag + inertia:";
    VNE_LOG_INFO << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: right-mouse pan ---
    manipulator->handleMouseButton(
        static_cast<int>(vne::interaction::MouseButton::eRight), /*pressed=*/true,
        640.0f, 360.0f, dt);
    for (int i = 0; i < 15; ++i) {
        manipulator->handleMouseMove(640.0f + i * 4.0f, 360.0f, 4.0f, 0.0f, dt);
    }
    manipulator->handleMouseButton(
        static_cast<int>(vne::interaction::MouseButton::eRight), /*pressed=*/false,
        640.0f + 15 * 4.0f, 360.0f, dt);

    // Decay pan inertia
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }
    VNE_LOG_INFO << "After pan + inertia decay:";
    VNE_LOG_INFO << "  coi=" << arcball->getCenterOfInterestWorld().x()
                 << ", " << arcball->getCenterOfInterestWorld().y()
                 << ", " << arcball->getCenterOfInterestWorld().z();

    // --- fitToAABB ---
    manipulator->fitToAABB({-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f});
    VNE_LOG_INFO << "After fitToAABB([-2,-2,-2] to [2,2,2]):";
    VNE_LOG_INFO << "  orbitDistance=" << arcball->getOrbitDistance()
                 << "  sceneScale=" << manipulator->getSceneScale();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
