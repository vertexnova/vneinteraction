/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 05 - Fly Manipulator
 * Demonstrates: creating a FlyManipulator (free-flight, no gravity/pitch
 * clamp), configuring speed settings, simulating 6-DOF WASD+QE flight,
 * and mouse-look.
 *
 * Key codes match GLFW: W=87, A=65, S=83, D=68, Q=81, E=69, Shift=340.
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // --- Create via factory ---
    const vne::interaction::CameraManipulatorFactory factory;
    auto manipulator = factory.create(vne::interaction::CameraManipulatorType::eFly);

    auto* fly = dynamic_cast<vne::interaction::FlyManipulator*>(manipulator.get());

    // --- Configure ---
    manipulator->setViewportSize(1280.0f, 720.0f);
    fly->setMoveSpeed(8.0f);
    fly->setMouseSensitivity(0.12f);
    fly->setSprintMultiplier(5.0f);
    fly->setSlowMultiplier(0.15f);
    fly->setZoomSpeed(0.5f);

    VNE_LOG_INFO << "FlyManipulator created";
    VNE_LOG_INFO << "  moveSpeed=" << fly->getMoveSpeed() << "  sensitivity=" << fly->getMouseSensitivity()
                 << "  sprintMult=" << fly->getSprintMultiplier();

    constexpr double dt = 1.0 / 60.0;

    // --- Simulate: W (forward) for 30 frames ---
    constexpr int key_w = 87;
    manipulator->handleKeyboard(key_w, /*pressed=*/true, dt);
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }
    manipulator->handleKeyboard(key_w, /*pressed=*/false, dt);
    VNE_LOG_INFO << "After 30 frames flying forward";

    // --- Simulate: Q (down) for 20 frames ---
    constexpr int key_q = 81;
    manipulator->handleKeyboard(key_q, /*pressed=*/true, dt);
    for (int i = 0; i < 20; ++i) {
        manipulator->update(dt);
    }
    manipulator->handleKeyboard(key_q, /*pressed=*/false, dt);
    VNE_LOG_INFO << "After 20 frames flying down";

    // --- Simulate: E (up) for 20 frames ---
    constexpr int key_e = 69;
    manipulator->handleKeyboard(key_e, /*pressed=*/true, dt);
    for (int i = 0; i < 20; ++i) {
        manipulator->update(dt);
    }
    manipulator->handleKeyboard(key_e, /*pressed=*/false, dt);
    VNE_LOG_INFO << "After 20 frames flying up";

    // --- Simulate: mouse-look pitch down then level ---
    manipulator->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eRight),
                                   /*pressed=*/true,
                                   640.0f,
                                   360.0f,
                                   dt);
    float my = 360.0f;
    for (int i = 0; i < 15; ++i) {
        const float dy = 4.0f;
        manipulator->handleMouseMove(640.0f, my + dy, 0.0f, dy, dt);
        my += dy;
    }
    manipulator->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eRight),
                                   /*pressed=*/false,
                                   640.0f,
                                   my,
                                   dt);
    VNE_LOG_INFO << "After mouse-look pitch";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: scroll to zoom ---
    manipulator->handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, dt);
    manipulator->handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, dt);
    VNE_LOG_INFO << "After scroll zoom";

    // --- fitToAABB and reset ---
    manipulator->fitToAABB({-10.0f, -2.0f, -10.0f}, {10.0f, 8.0f, 10.0f});
    VNE_LOG_INFO << "After fitToAABB:  sceneScale=" << manipulator->getSceneScale();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
