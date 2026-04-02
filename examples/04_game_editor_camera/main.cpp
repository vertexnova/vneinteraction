/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 04: Game / editor camera — Navigation3DController
 *
 * Demonstrates:
 *   - FPS mode (world-up constrained, pitch clamped ±89°)
 *   - Fly mode (unconstrained 6-DoF)
 *   - WASD + look, sprint, slow modifiers
 *   - Full 6-DoF key bindings: up/down (E/Q by default)
 *   - Key rebinding (arrow keys, custom look button)
 *   - DOF gating: setLookEnabled / setMoveEnabled / setZoomEnabled
 *   - Discrete speed-step keys (]/[ to adjust move speed in-flight)
 *   - freeLookManipulator() escape hatch for direct tuning
 *   - ZoomMethod variants
 *   - fitToAABB and reset()
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

static constexpr double kDt  = 1.0 / 60.0;
static constexpr float  kVpW = 1280.0f;
static constexpr float  kVpH = 720.0f;
static constexpr float  kCx  = kVpW / 2.0f;
static constexpr float  kCy  = kVpH / 2.0f;

static void runUpdate(vne::interaction::Navigation3DController& ctrl,
                      std::shared_ptr<vne::scene::ICamera> camera,
                      int frames,
                      const char* label) {
    for (int i = 0; i < frames; ++i) ctrl.onUpdate(kDt);
    const auto pos = camera->getPosition();
    VNE_LOG_INFO << label << " — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
}

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // ── Camera ────────────────────────────────────────────────────────────────
    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(60.0f, kVpW / kVpH, 0.1f, 2000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 10.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 1.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // ── Controller ────────────────────────────────────────────────────────────
    vne::interaction::Navigation3DController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(kVpW, kVpH);

    auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: FPS mode — basic WASD + mouse look
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: FPS mode, WASD + mouse look ---";
    ctrl.setMode(vne::interaction::FreeLookMode::eFps);
    ctrl.setMoveSpeed(8.0f);
    ctrl.setMouseSensitivity(0.18f);
    ctrl.setSprintMultiplier(4.0f);
    ctrl.setSlowMultiplier(0.15f);

    VNE_LOG_INFO << "  mode=" << (ctrl.getMode() == vne::interaction::FreeLookMode::eFps ? "eFps" : "eFly")
                 << "  speed=" << ctrl.getMoveSpeed()
                 << "  sensitivity=" << ctrl.getMouseSensitivity()
                 << "  sprint=" << ctrl.getSprintMultiplier()
                 << "  slow=" << ctrl.getSlowMultiplier();

    // RMB press → begin look
    vne::events::MouseButtonPressedEvent rmb_press(
        vne::events::MouseButton::eRight, 0,
        static_cast<double>(kCx), static_cast<double>(kCy));
    on_event(rmb_press, kDt);

    // Mouse moved → look delta
    vne::events::MouseMovedEvent look_move(
        static_cast<double>(kCx + 30.0f), static_cast<double>(kCy - 15.0f));
    on_event(look_move, kDt);

    // W held for 30 frames → move forward
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 30, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    runUpdate(ctrl, camera, 5, "FPS: W held 30 frames");

    // A held → strafe left
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eA, 20, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    runUpdate(ctrl, camera, 5, "FPS: A strafe");

    // E held → move up
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eE, 15, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    runUpdate(ctrl, camera, 5, "FPS: E move up");

    // Q held → move down
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eQ, 15, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    runUpdate(ctrl, camera, 5, "FPS: Q move down");

    // RMB release → end look
    vne::events::MouseButtonReleasedEvent rmb_release(
        vne::events::MouseButton::eRight, 0,
        static_cast<double>(kCx + 30.0f), static_cast<double>(kCy - 15.0f));
    on_event(rmb_release, kDt);

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: Sprint and slow modifiers
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: Sprint (Shift) and slow (Ctrl) ---";
    on_event(rmb_press, kDt);

    // Shift + W = sprint
    vne::events::KeyPressedEvent shift_press(vne::events::KeyCode::eLeftShift);
    on_event(shift_press, kDt);
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 20, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    vne::events::KeyReleasedEvent shift_release(vne::events::KeyCode::eLeftShift);
    on_event(shift_release, kDt);
    runUpdate(ctrl, camera, 3, "Sprint W (Shift held)");

    // Ctrl + W = slow
    vne::events::KeyPressedEvent ctrl_press(vne::events::KeyCode::eLeftControl);
    on_event(ctrl_press, kDt);
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 20, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    vne::events::KeyReleasedEvent ctrl_release(vne::events::KeyCode::eLeftControl);
    on_event(ctrl_release, kDt);
    runUpdate(ctrl, camera, 3, "Slow-walk W (Ctrl held)");

    on_event(rmb_release, kDt);

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: Fly mode — unconstrained 6-DoF
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: Fly mode (no world-up constraint) ---";
    ctrl.setMode(vne::interaction::FreeLookMode::eFly);
    VNE_LOG_INFO << "  mode=" << (ctrl.getMode() == vne::interaction::FreeLookMode::eFps ? "eFps" : "eFly");

    on_event(rmb_press, kDt);
    // Large vertical mouse delta: in Fly mode pitch can exceed ±89°
    vne::events::MouseMovedEvent fly_look(
        static_cast<double>(kCx), static_cast<double>(kCy - 200.0f));
    on_event(fly_look, kDt);
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 20, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    on_event(rmb_release, kDt);
    runUpdate(ctrl, camera, 5, "Fly mode: pitched past 90°");

    ctrl.setMode(vne::interaction::FreeLookMode::eFps);  // restore

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: Key rebinding (arrow keys layout)
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: Rebind to arrow keys ---";
    ctrl.setMoveForwardKey(vne::events::KeyCode::eUp);
    ctrl.setMoveBackwardKey(vne::events::KeyCode::eDown);
    ctrl.setMoveLeftKey(vne::events::KeyCode::eLeft);
    ctrl.setMoveRightKey(vne::events::KeyCode::eRight);
    // Vertical movement keys
    ctrl.setMoveUpKey(vne::events::KeyCode::ePageUp);
    ctrl.setMoveDownKey(vne::events::KeyCode::ePageDown);
    // Override sprint to a single key
    ctrl.setSpeedBoostKey(vne::events::KeyCode::eRightShift);
    // Override look to LMB
    ctrl.setLookButton(vne::events::MouseButton::eLeft);

    vne::events::MouseButtonPressedEvent lmb_press(
        vne::events::MouseButton::eLeft, 0,
        static_cast<double>(kCx), static_cast<double>(kCy));
    on_event(lmb_press, kDt);
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eUp, 20, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    vne::events::MouseButtonReleasedEvent lmb_release(
        vne::events::MouseButton::eLeft, 0,
        static_cast<double>(kCx), static_cast<double>(kCy));
    on_event(lmb_release, kDt);
    runUpdate(ctrl, camera, 5, "Arrow key forward with LMB look");

    // Restore WASD defaults
    ctrl.setMoveForwardKey(vne::events::KeyCode::eW);
    ctrl.setMoveBackwardKey(vne::events::KeyCode::eS);
    ctrl.setMoveLeftKey(vne::events::KeyCode::eA);
    ctrl.setMoveRightKey(vne::events::KeyCode::eD);
    ctrl.setMoveUpKey(vne::events::KeyCode::eE);
    ctrl.setMoveDownKey(vne::events::KeyCode::eQ);
    ctrl.setSpeedBoostKey(vne::events::KeyCode::eUnknown);  // restore default Shift pair
    ctrl.setLookButton(vne::events::MouseButton::eRight);

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: DOF gating
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: DOF gating ---";

    ctrl.setMoveEnabled(false);
    VNE_LOG_INFO << "  move_enabled=" << ctrl.isMoveEnabled();
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 10, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    runUpdate(ctrl, camera, 3, "Move disabled — W ignored");
    ctrl.setMoveEnabled(true);

    ctrl.setLookEnabled(false);
    VNE_LOG_INFO << "  look_enabled=" << ctrl.isLookEnabled();
    on_event(rmb_press, kDt);
    vne::events::MouseMovedEvent blocked_look(
        static_cast<double>(kCx + 100.0f), static_cast<double>(kCy));
    on_event(blocked_look, kDt);
    on_event(rmb_release, kDt);
    runUpdate(ctrl, camera, 3, "Look disabled — mouse delta ignored");
    ctrl.setLookEnabled(true);

    ctrl.setZoomEnabled(false);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 3, "Zoom disabled — scroll ignored");
    ctrl.setZoomEnabled(true);

    // ─────────────────────────────────────────────────────────────────────────
    // Section F: Discrete speed-step keys
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- F: Discrete speed-step keys (]/[) ---";
    ctrl.setIncreaseMoveSpeedKey(vne::events::KeyCode::eRightBracket);
    ctrl.setDecreaseMoveSpeedKey(vne::events::KeyCode::eLeftBracket);
    ctrl.setMoveSpeedStep(2.0f);   // multiplicative factor per key press
    ctrl.setMoveSpeedMin(0.5f);
    ctrl.setMoveSpeedMax(500.0f);

    VNE_LOG_INFO << "  speed before steps=" << ctrl.getMoveSpeed();
    // ] → increase speed twice
    for (int i = 0; i < 2; ++i) {
        vne::interaction::examples::simulateKeyHold(on_event,
            vne::events::KeyCode::eRightBracket, 1, kDt,
            [&](double dt) { ctrl.onUpdate(dt); });
    }
    VNE_LOG_INFO << "  speed after 2× ]=" << ctrl.getMoveSpeed();

    // [ → decrease speed once
    vne::interaction::examples::simulateKeyHold(on_event,
        vne::events::KeyCode::eLeftBracket, 1, kDt,
        [&](double dt) { ctrl.onUpdate(dt); });
    VNE_LOG_INFO << "  speed after 1× [=" << ctrl.getMoveSpeed();

    // ─────────────────────────────────────────────────────────────────────────
    // Section G: freeLookManipulator() escape hatch
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- G: freeLookManipulator() escape hatch ---";
    auto& manip = ctrl.freeLookManipulator();

    VNE_LOG_INFO << "  handle_zoom=" << manip.getHandleZoom()
                 << "  world_up=(" << manip.getWorldUp().x() << "," << manip.getWorldUp().y() << ","
                 << manip.getWorldUp().z() << ")";

    // Z-up world (CAD/scientific convention)
    manip.setWorldUp(vne::math::Vec3f(0.0f, 0.0f, 1.0f));
    VNE_LOG_INFO << "  world_up set to Z-up";

    // Disable zoom on this manipulator (another rig member would own zoom)
    manip.setHandleZoom(false);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 3, "handleZoom=false — scroll not applied");
    manip.setHandleZoom(true);

    // Restore Y-up
    manip.setWorldUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // ─────────────────────────────────────────────────────────────────────────
    // Section H: ZoomMethod via escape hatch
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- H: ZoomMethod variants ---";
    manip.setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 3, "ZoomMethod::eSceneScale");

    manip.setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 3, "ZoomMethod::eChangeFov");

    manip.setZoomMethod(vne::interaction::ZoomMethod::eDollyToCoi);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 3, "ZoomMethod::eDollyToCoi");

    // ─────────────────────────────────────────────────────────────────────────
    // Section I: fitToAABB and reset
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- I: fitToAABB + reset ---";
    ctrl.fitToAABB(vne::math::Vec3f(-20.0f, 0.0f, -20.0f),
                   vne::math::Vec3f( 20.0f, 10.0f,  20.0f));
    runUpdate(ctrl, camera, 10, "After fitToAABB (scene bounds)");

    ctrl.reset();
    runUpdate(ctrl, camera, 5, "After reset()");

    VNE_LOG_INFO << "04_game_editor_camera: done.";
    return 0;
}
