/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 07 - Follow Manipulator
 * Demonstrates: creating a FollowManipulator that tracks a moving target.
 * Shows both the static target API (setTargetWorld) and the dynamic
 * provider API (setTargetProvider) with a simulated moving object.
 *
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/input_simulation.h"
#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <cmath>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;
    using namespace vne::interaction::examples;

    // --- Create via factory ---
    const vne::interaction::CameraManipulatorFactory factory;
    auto manipulator = factory.create(vne::interaction::CameraManipulatorType::eFollow);

    auto* follow = dynamic_cast<vne::interaction::FollowManipulator*>(manipulator.get());

    // --- Configure ---
    manipulator->setViewportSize(1280.0f, 720.0f);
    follow->setOffsetWorld({0.0f, 3.0f, 8.0f});  // camera sits 3 up, 8 behind target
    follow->setDamping(6.0f);
    follow->setZoomSpeed(1.1f);

    VNE_LOG_INFO << "FollowManipulator created";
    VNE_LOG_INFO << "  offsetWorld=(" << follow->getOffsetWorld().x() << ", " << follow->getOffsetWorld().y() << ", "
                 << follow->getOffsetWorld().z() << ")"
                 << "  damping=" << follow->getDamping();

    constexpr double dt = 1.0 / 60.0;

    // --- Static target: point camera at a fixed world position ---
    follow->setTargetWorld({0.0f, 0.0f, 0.0f});
    for (int i = 0; i < 10; ++i) {
        manipulator->update(dt);
    }
    VNE_LOG_INFO << "Static target (0,0,0), 10 frames:";
    VNE_LOG_INFO << "  targetWorld=(" << follow->getTargetWorld().x() << ", " << follow->getTargetWorld().y() << ", "
                 << follow->getTargetWorld().z() << ")";

    // --- Dynamic provider: target moves in a circle on the XZ plane ---
    float time = 0.0f;
    auto target_provider = [&time]() -> vne::math::Vec3f {
        return {std::cos(time) * 5.0f, 0.0f, std::sin(time) * 5.0f};
    };
    follow->setTargetProvider(target_provider);

    VNE_LOG_INFO << "Dynamic target: circular motion, simulating 120 frames";
    for (int i = 0; i < 120; ++i) {
        time += static_cast<float>(dt);
        manipulator->update(dt);
    }
    const auto target = follow->getTargetWorld();
    VNE_LOG_INFO << "After 120 frames:";
    VNE_LOG_INFO << "  targetWorld=(" << target.x() << ", " << target.y() << ", " << target.z() << ")";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Scroll to adjust zoom distance ---
    simulateMouseScroll(*manipulator, -1.0f, 640.0f, 360.0f, 2, dt);
    VNE_LOG_INFO << "After 2x zoom in:  sceneScale=" << manipulator->getSceneScale();

    // --- fitToAABB ---
    manipulator->fitToAABB({-5.0f, -1.0f, -5.0f}, {5.0f, 1.0f, 5.0f});
    VNE_LOG_INFO << "After fitToAABB:  sceneScale=" << manipulator->getSceneScale();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
