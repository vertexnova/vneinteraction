/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 05: Robotic simulator — InspectController (robot/tool inspection),
 * Navigation3DController (environment), FollowController (end-effector view).
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/follow_controller.h"
#include "vertexnova/interaction/inspect_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 6.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // 1. InspectController — inspect robot arm / tool / anatomy
    vne::interaction::InspectController inspect;
    inspect.setCamera(camera);
    inspect.onResize(1280.0f, 720.0f);
    inspect.setPivot(vne::math::Vec3f(0.0f, 0.5f, 0.0f));  // robot base
    inspect.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);

    auto on_inspect = [&inspect](const vne::events::Event& e, double dt) { inspect.onEvent(e, dt); };

    constexpr double dt = 1.0 / 60.0;
    vne::interaction::examples::simulateMouseDrag(on_inspect,
                                                  vne::events::MouseButton::eLeft,
                                                  640.0f,
                                                  360.0f,
                                                  60.0f,
                                                  40.0f,
                                                  15,
                                                  dt);

    for (int i = 0; i < 15; ++i) {
        inspect.onUpdate(dt);
    }

    // 2. Navigation3DController — move through environment
    vne::interaction::Navigation3DController navigate;
    navigate.setCamera(camera);
    navigate.onResize(1280.0f, 720.0f);
    navigate.setMode(vne::interaction::NavigateMode::eFps);

    // 3. FollowController — end-effector follow-cam (FollowBehavior)
    vne::interaction::FollowController follow;
    follow.setCamera(camera);
    follow.onResize(1280.0f, 720.0f);
    vne::math::Mat4f target_xform = vne::math::Mat4f::translate(vne::math::Vec3f(0.5f, 0.8f, 0.2f));
    follow.setTarget(target_xform);
    follow.setLag(0.15f);

    for (int i = 0; i < 30; ++i) {
        follow.onUpdate(dt);
    }

    VNE_LOG_INFO << "Robotic simulator: InspectController + Navigation3DController + FollowController — done.";
    return 0;
}
