/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 02: Medical 3D inspect — InspectController with arcball,
 * landmark pivot, fitToAABB, DOF toggles.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/inspect_controller.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::InspectController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(1280.0f, 720.0f);

    // Default: arcball (quaternion), eCoi pivot
    VNE_LOG_INFO << "Rotation mode: "
                 << (ctrl.getRotationMode() == vne::interaction::OrbitRotationMode::eTrackball ? "arcball" : "orbit");

    // Landmark-centered inspection (medical use case)
    ctrl.setPivot(vne::math::Vec3f(0.5f, 0.3f, 0.0f));  // e.g. anatomical landmark
    ctrl.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);

    auto on_event = [&ctrl](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

    constexpr double dt = 1.0 / 60.0;
    const float cx = 640.0f;
    const float cy = 360.0f;

    // Simulate LMB drag (rotate)
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  cx,
                                                  cy,
                                                  80.0f,
                                                  40.0f,
                                                  20,
                                                  dt);

    // Simulate scroll zoom
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, cx, cy, 3, dt);

    // Inertia decay
    for (int i = 0; i < 30; ++i) {
        ctrl.onUpdate(dt);
    }

    // Fit to AABB (e.g. anatomy bounding box)
    ctrl.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));

    for (int i = 0; i < 60; ++i) {
        ctrl.onUpdate(dt);
    }

    VNE_LOG_INFO << "Medical 3D inspect: trackball, fixed pivot, fitToAABB — done.";
    return 0;
}
