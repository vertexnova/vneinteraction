/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 04: Game/editor camera — Navigation3DController FPS and Fly
 * (WASD + look), sprint. Orbit inspection: use Inspect3DController.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

#include "vertexnova/events/key_event.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 8.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::Navigation3DController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(1280.0f, 720.0f);
    ctrl.setMoveSpeed(5.0f);
    ctrl.setSprintMultiplier(3.0f);

    auto on_event = [&ctrl](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

    constexpr double dt = 1.0 / 60.0;
    const float cx = 640.0f;
    const float cy = 360.0f;

    // FPS mode: WASD + look
    ctrl.setMode(vne::interaction::NavigateMode::eFps);
    vne::events::MouseButtonPressedEvent look_press(vne::events::MouseButton::eRight,
                                                    0,
                                                    static_cast<double>(cx),
                                                    static_cast<double>(cy));
    on_event(look_press, dt);
    vne::events::MouseMovedEvent look_move(650.0, 355.0);
    on_event(look_move, dt);
    vne::events::KeyPressedEvent w_press(vne::events::KeyCode::eW);
    on_event(w_press, dt);
    for (int i = 0; i < 20; ++i) {
        ctrl.onUpdate(dt);
    }
    vne::events::KeyReleasedEvent w_release(vne::events::KeyCode::eW);
    on_event(w_release, dt);
    vne::events::MouseButtonReleasedEvent look_release(vne::events::MouseButton::eRight, 0, 650.0, 355.0);
    on_event(look_release, dt);

    // Fly mode: same input preset (RMB look, WASD); unconstrained up / roll
    ctrl.setMode(vne::interaction::NavigateMode::eFly);
    on_event(look_press, dt);
    on_event(look_move, dt);
    for (int i = 0; i < 10; ++i) {
        ctrl.onUpdate(dt);
    }
    on_event(look_release, dt);

    VNE_LOG_INFO << "Game editor camera: FPS + Fly navigation, WASD — done.";
    return 0;
}
