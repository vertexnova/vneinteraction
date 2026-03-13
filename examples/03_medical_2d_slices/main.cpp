/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 03: Medical 2D slices — Ortho2DController for ortho pan+zoom.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_2d_controller.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    auto camera = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));

    vne::interaction::Ortho2DController ctrl;
    ctrl.setCamera(camera);
    ctrl.setViewportSize(512.0f, 512.0f);

    auto on_event = [&ctrl](const vne::events::Event& e, double) { ctrl.onEvent(e); };

    constexpr double dt = 1.0 / 60.0;
    const float cx = 256.0f;
    const float cy = 256.0f;

    // Simulate RMB drag (pan)
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eRight,
                                                  cx,
                                                  cy,
                                                  -30.0f,
                                                  20.0f,
                                                  15,
                                                  dt);

    // Simulate scroll zoom
    vne::interaction::examples::simulateMouseScroll(on_event, -0.5f, cx, cy, 2, dt);

    for (int i = 0; i < 20; ++i) {
        ctrl.onUpdate(dt);
    }

    // Fit to slice AABB
    ctrl.fitToAABB(vne::math::Vec3f(-2.0f, -2.0f, 0.0f), vne::math::Vec3f(2.0f, 2.0f, 0.0f));

    for (int i = 0; i < 30; ++i) {
        ctrl.onUpdate(dt);
    }

    VNE_LOG_INFO << "Medical 2D slices: pan, zoom, fitToAABB — done.";
    return 0;
}
