/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 04 - FPS Manipulator
 * Demonstrates: creating an FpsManipulator, configuring movement/look
 * settings, and simulating keyboard WASD movement + right-mouse look.
 *
 * Key codes match GLFW: W=87, A=65, S=83, D=68, Q=81, E=69, Shift=340.
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // --- Create via factory ---
    const vne::interaction::CameraManipulatorFactory factory;
    auto manipulator = factory.create(vne::interaction::CameraManipulatorType::eFps);

    auto* fps = dynamic_cast<vne::interaction::FpsManipulator*>(manipulator.get());

    // --- Configure ---
    manipulator->setViewportSize(1280.0f, 720.0f);
    fps->setMoveSpeed(5.0f);
    fps->setMouseSensitivity(0.15f);
    fps->setSprintMultiplier(4.0f);
    fps->setSlowMultiplier(0.2f);
    fps->setZoomSpeed(0.5f);

    VNE_LOG_INFO << "FpsManipulator created";
    VNE_LOG_INFO << "  moveSpeed=" << fps->getMoveSpeed() << "  sensitivity=" << fps->getMouseSensitivity()
                 << "  sprintMult=" << fps->getSprintMultiplier();

    constexpr double dt = 1.0 / 60.0;

    // --- Simulate: hold W (forward) for 60 frames ---
    constexpr int key_w = 87;
    manipulator->handleKeyboard(key_w, /*pressed=*/true, dt);
    for (int i = 0; i < 60; ++i) {
        manipulator->update(dt);
    }
    manipulator->handleKeyboard(key_w, /*pressed=*/false, dt);
    VNE_LOG_INFO << "After 60 frames walking forward:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale();

    // --- Simulate: hold Shift + W (sprint) for 30 frames ---
    constexpr int key_shift = 340;
    manipulator->handleKeyboard(key_shift, /*pressed=*/true, dt);
    manipulator->handleKeyboard(key_w, /*pressed=*/true, dt);
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }
    manipulator->handleKeyboard(key_w, /*pressed=*/false, dt);
    manipulator->handleKeyboard(key_shift, /*pressed=*/false, dt);
    VNE_LOG_INFO << "After 30 frames sprinting forward";

    // --- Simulate: right-mouse look (activate look mode then drag) ---
    manipulator->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eRight),
                                   /*pressed=*/true,
                                   640.0f,
                                   360.0f,
                                   dt);
    float mx = 640.0f;
    for (int i = 0; i < 20; ++i) {
        const float dx = 3.0f;
        manipulator->handleMouseMove(mx + dx, 360.0f, dx, 0.0f, dt);
        mx += dx;
    }
    manipulator->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eRight),
                                   /*pressed=*/false,
                                   mx,
                                   360.0f,
                                   dt);
    VNE_LOG_INFO << "After mouse-look yaw";

    // --- Simulate: strafe left (A) + move up (E) ---
    constexpr int key_a = 65;
    constexpr int key_e = 69;
    manipulator->handleKeyboard(key_a, /*pressed=*/true, dt);
    manipulator->handleKeyboard(key_e, /*pressed=*/true, dt);
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }
    manipulator->handleKeyboard(key_a, /*pressed=*/false, dt);
    manipulator->handleKeyboard(key_e, /*pressed=*/false, dt);
    VNE_LOG_INFO << "After strafe-left + move-up";

    // --- fitToAABB then reset ---
    manipulator->fitToAABB({-5.0f, 0.0f, -5.0f}, {5.0f, 3.0f, 5.0f});
    VNE_LOG_INFO << "After fitToAABB:  sceneScale=" << manipulator->getSceneScale();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
