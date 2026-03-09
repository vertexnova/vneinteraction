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

#include "common/input_simulation.h"
#include "common/key_codes.h"
#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;
    using namespace vne::interaction::examples;

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
    simulateKeyHold(*manipulator, key_w, 60, dt);
    VNE_LOG_INFO << "After 60 frames walking forward:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale();

    // --- Simulate: hold Shift+W (sprint) for 30 frames ---
    simulateKeyHoldMulti(*manipulator, {key_shift, key_w}, 30, dt);
    VNE_LOG_INFO << "After 30 frames sprinting forward";

    // --- Simulate: right-mouse look (20 frames × dx=3) ---
    simulateMouseDrag(*manipulator,
                      vne::interaction::MouseButton::eRight,
                      640.0f,
                      360.0f,
                      /*total_dx=*/60.0f,
                      /*total_dy=*/0.0f,
                      20,
                      dt);
    VNE_LOG_INFO << "After mouse-look yaw";

    // --- Simulate: strafe left (A) + move up (E) ---
    simulateKeyHoldMulti(*manipulator, {key_a, key_e}, 30, dt);
    VNE_LOG_INFO << "After strafe-left + move-up";

    // --- fitToAABB then reset ---
    manipulator->fitToAABB({-5.0f, 0.0f, -5.0f}, {5.0f, 3.0f, 5.0f});
    VNE_LOG_INFO << "After fitToAABB:  sceneScale=" << manipulator->getSceneScale();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
