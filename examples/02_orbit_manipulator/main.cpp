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

#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/orbit_manipulator.h"

#include <vertexnova/math/core/core.h>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

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
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance()
                 << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: press left mouse button to begin orbit ---
    constexpr double dt = 1.0 / 60.0;
    manipulator->handleMouseButton(
        static_cast<int>(vne::interaction::MouseButton::eLeft), /*pressed=*/true,
        640.0f, 360.0f, dt);

    // --- Simulate: drag horizontally (orbit left/right) ---
    float mouse_x = 640.0f;
    float mouse_y = 360.0f;
    for (int i = 0; i < 30; ++i) {
        const float dx = 2.0f;
        manipulator->handleMouseMove(mouse_x + dx, mouse_y, dx, 0.0f, dt);
        mouse_x += dx;
    }

    // --- Release mouse (triggers inertia) ---
    manipulator->handleMouseButton(
        static_cast<int>(vne::interaction::MouseButton::eLeft), /*pressed=*/false,
        mouse_x, mouse_y, dt);

    // --- Let inertia decay over 30 frames ---
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }

    VNE_LOG_INFO << "After orbit + inertia decay:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate scroll zoom ---
    manipulator->handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, dt);
    manipulator->handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, dt);
    VNE_LOG_INFO << "After 2x scroll-zoom in:";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance();

    // --- Fit to a unit AABB ---
    const vne::math::Vec3f aabb_min{-1.0f, -1.0f, -1.0f};
    const vne::math::Vec3f aabb_max{ 1.0f,  1.0f,  1.0f};
    manipulator->fitToAABB(aabb_min, aabb_max);
    VNE_LOG_INFO << "After fitToAABB([-1,-1,-1] to [1,1,1]):";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance()
                 << "  sceneScale=" << manipulator->getSceneScale();

    // --- Reset ---
    manipulator->resetState();
    VNE_LOG_INFO << "After resetState:";
    VNE_LOG_INFO << "  orbitDistance=" << orbit->getOrbitDistance();

    return 0;
}
