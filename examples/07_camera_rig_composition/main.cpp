/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 07: CameraRig composition
 *
 * Demonstrates:
 *   - CameraRig factory methods (makeOrbit, makeTrackball, makeFps, makeFly,
 *     makeOrtho2D, makeFollow)
 *   - addManipulator() for a hybrid orbit + fly rig
 *   - removeManipulator() to hot-swap a manipulator at runtime
 *   - clearManipulators() and manual rebuild
 *   - setEnabled() on individual manipulators (mute one, keep the other)
 *   - setHandleZoom(false) on FreeLookManipulator to avoid double-zoom
 *   - Direct onAction() dispatch (bypass controller / mapper)
 *   - resetState() on the rig
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

static constexpr double kDt  = 1.0 / 60.0;
static constexpr float  kVpW = 1280.0f;
static constexpr float  kVpH = 720.0f;
static constexpr float  kCx  = kVpW / 2.0f;
static constexpr float  kCy  = kVpH / 2.0f;

// Helper: emit one complete orbit-drag action sequence directly to a rig.
static void directOrbitDrag(vne::interaction::CameraRig& rig, float dx, float dy) {
    using A = vne::interaction::CameraActionType;
    vne::interaction::CameraCommandPayload p;

    p.x_px = kCx; p.y_px = kCy; p.pressed = true;
    rig.onAction(A::eBeginRotate, p, kDt);

    p.delta_x_px = dx; p.delta_y_px = dy;
    rig.onAction(A::eRotateDelta, p, kDt);

    p.pressed = false;
    rig.onAction(A::eEndRotate, p, kDt);
}

static void directZoom(vne::interaction::CameraRig& rig, float factor) {
    vne::interaction::CameraCommandPayload p;
    p.x_px = kCx; p.y_px = kCy;
    p.zoom_factor = factor;
    rig.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, kDt);
}

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, kVpW / kVpH, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 8.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: Factory methods
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: CameraRig factory methods ---";
    {
        auto r_orbit  = vne::interaction::CameraRig::makeOrbit();
        auto r_tb     = vne::interaction::CameraRig::makeTrackball();
        auto r_fps    = vne::interaction::CameraRig::makeFps();
        auto r_fly    = vne::interaction::CameraRig::makeFly();
        auto r_ortho  = vne::interaction::CameraRig::makeOrtho2D();
        auto r_follow = vne::interaction::CameraRig::makeFollow();

        VNE_LOG_INFO << "  orbit="   << r_orbit.manipulators().size()
                     << "  tb="      << r_tb.manipulators().size()
                     << "  fps="     << r_fps.manipulators().size()
                     << "  fly="     << r_fly.manipulators().size()
                     << "  ortho2d=" << r_ortho.manipulators().size()
                     << "  follow="  << r_follow.manipulators().size();

        // Simple orbit factory drive via direct actions
        r_orbit.setCamera(camera);
        r_orbit.onResize(kVpW, kVpH);
        directOrbitDrag(r_orbit, 60.0f, 20.0f);
        r_orbit.onUpdate(kDt);
        directZoom(r_orbit, 0.9f);  // 0.9 < 1 = zoom in
        r_orbit.onUpdate(kDt);
        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  makeOrbit direct drive — eye=("
                     << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: Hybrid rig — orbit + fly in the same rig
    //   Both manipulators receive every action.
    //   OrbitalCameraManipulator handles eBeginRotate/eRotateDelta/eZoomAtCursor.
    //   FreeLookManipulator handles eMoveForward/eLookDelta/eZoomAtCursor.
    //   setHandleZoom(false) on FreeLook prevents double-zoom per scroll tick.
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: Hybrid orbit + fly rig ---";
    {
        // Build orbit base manipulator
        auto orbit_manip = std::make_shared<vne::interaction::OrbitalCameraManipulator>();
        orbit_manip->setRotationMode(vne::interaction::OrbitalRotationMode::eOrbit);
        orbit_manip->setRotationInertiaEnabled(true);
        orbit_manip->setRotationDamping(5.0f);

        // Build fly manipulator — do NOT let it own zoom (orbit does)
        auto fly_manip = std::make_shared<vne::interaction::FreeLookManipulator>();
        fly_manip->setMode(vne::interaction::FreeLookMode::eFly);
        fly_manip->setMoveSpeed(5.0f);
        fly_manip->setHandleZoom(false);  // orbit owns eZoomAtCursor

        // Compose rig
        vne::interaction::CameraRig rig;
        rig.addManipulator(orbit_manip);
        rig.addManipulator(fly_manip);
        rig.setCamera(camera);
        rig.onResize(kVpW, kVpH);

        VNE_LOG_INFO << "  manipulators=" << rig.manipulators().size();

        // Wire InputMapper with gamePreset (LMB=orbit, RMB=look, WASD=move, scroll=zoom)
        vne::interaction::InputMapper mapper;
        mapper.setRules(vne::interaction::InputMapper::gamePreset());
        mapper.setActionCallback(
            [&rig](vne::interaction::CameraActionType action,
                   const vne::interaction::CameraCommandPayload& payload,
                   double dt) {
                rig.onAction(action, payload, dt);
            });

        auto on_event = [&](const vne::events::Event& e, double dt) {
            using ET = vne::events::EventType;
            switch (e.type()) {
                case ET::eMouseButtonPressed: {
                    const auto& me = static_cast<const vne::events::MouseButtonPressedEvent&>(e);
                    mapper.onMouseButton(static_cast<int>(me.button()), true,
                                         static_cast<float>(me.x()),
                                         static_cast<float>(me.y()), dt);
                    break;
                }
                case ET::eMouseButtonReleased: {
                    const auto& me = static_cast<const vne::events::MouseButtonReleasedEvent&>(e);
                    mapper.onMouseButton(static_cast<int>(me.button()), false,
                                         static_cast<float>(me.x()),
                                         static_cast<float>(me.y()), dt);
                    break;
                }
                case ET::eMouseMoved: {
                    const auto& me = static_cast<const vne::events::MouseMovedEvent&>(e);
                    mapper.onMouseMove(static_cast<float>(me.x()),
                                       static_cast<float>(me.y()),
                                       0.0f, 0.0f, dt);
                    break;
                }
                case ET::eMouseScrolled: {
                    const auto& me = static_cast<const vne::events::MouseScrolledEvent&>(e);
                    mapper.onMouseScroll(static_cast<float>(me.x_offset()),
                                         static_cast<float>(me.y_offset()),
                                         kCx, kCy, dt);
                    break;
                }
                case ET::eKeyPressed: {
                    const auto& ke = static_cast<const vne::events::KeyPressedEvent&>(e);
                    mapper.onKey(static_cast<int>(ke.key_code()), true, dt);
                    break;
                }
                case ET::eKeyReleased: {
                    const auto& ke = static_cast<const vne::events::KeyReleasedEvent&>(e);
                    mapper.onKey(static_cast<int>(ke.key_code()), false, dt);
                    break;
                }
                default:
                    break;
            }
        };

        // LMB orbit
        vne::interaction::examples::simulateMouseDrag(on_event,
            vne::events::MouseButton::eLeft, kCx, kCy, 80.0f, 30.0f, 20, kDt);
        for (int i = 0; i < 15; ++i) rig.onUpdate(kDt);

        // WASD fly while orbit is also active (fly takes effect; orbit ignores move actions)
        vne::events::MouseButtonPressedEvent rmb(vne::events::MouseButton::eRight, 0,
            static_cast<double>(kCx), static_cast<double>(kCy));
        on_event(rmb, kDt);
        vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 20, kDt,
            [&](double dt) { rig.onUpdate(dt); });
        vne::events::MouseButtonReleasedEvent rmb_rel(vne::events::MouseButton::eRight, 0,
            static_cast<double>(kCx), static_cast<double>(kCy));
        on_event(rmb_rel, kDt);

        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  Hybrid rig (orbit+fly): eye=("
                     << pos.x() << "," << pos.y() << "," << pos.z() << ")";

        // ── removeManipulator: hot-remove fly at runtime ───────────────────
        rig.removeManipulator(fly_manip);
        VNE_LOG_INFO << "  After removeManipulator(fly): count=" << rig.manipulators().size();
        directOrbitDrag(rig, 30.0f, 0.0f);
        rig.onUpdate(kDt);
        VNE_LOG_INFO << "  Orbit still works after fly removed";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: setEnabled() — mute individual manipulators
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: setEnabled() per manipulator ---";
    {
        auto orbit_manip = std::make_shared<vne::interaction::OrbitalCameraManipulator>();
        auto fly_manip   = std::make_shared<vne::interaction::FreeLookManipulator>();
        fly_manip->setHandleZoom(false);

        vne::interaction::CameraRig rig;
        rig.addManipulator(orbit_manip);
        rig.addManipulator(fly_manip);
        rig.setCamera(camera);
        rig.onResize(kVpW, kVpH);

        // Disable fly — only orbit responds
        fly_manip->setEnabled(false);
        VNE_LOG_INFO << "  fly enabled=" << fly_manip->isEnabled()
                     << "  orbit enabled=" << orbit_manip->isEnabled();

        directOrbitDrag(rig, 50.0f, 10.0f);
        rig.onUpdate(kDt);
        VNE_LOG_INFO << "  Orbit active with fly muted";

        // Re-enable fly, disable orbit
        fly_manip->setEnabled(true);
        orbit_manip->setEnabled(false);

        using A = vne::interaction::CameraActionType;
        vne::interaction::CameraCommandPayload p;
        p.pressed = true;
        rig.onAction(A::eBeginLook, p, kDt);
        p.delta_x_px = 10.0f; p.delta_y_px = -5.0f;
        rig.onAction(A::eLookDelta, p, kDt);
        p.pressed = false;
        rig.onAction(A::eEndLook, p, kDt);
        rig.onUpdate(kDt);
        VNE_LOG_INFO << "  Fly active with orbit muted";

        orbit_manip->setEnabled(true);  // restore
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: clearManipulators() and manual rebuild
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: clearManipulators + rebuild ---";
    {
        vne::interaction::CameraRig rig;
        rig.setCamera(camera);
        rig.onResize(kVpW, kVpH);

        // Start with orbit
        auto m1 = std::make_shared<vne::interaction::OrbitalCameraManipulator>();
        rig.addManipulator(m1);
        VNE_LOG_INFO << "  After addManipulator(orbit): count=" << rig.manipulators().size();

        // Clear and rebuild with trackball
        rig.clearManipulators();
        VNE_LOG_INFO << "  After clearManipulators(): count=" << rig.manipulators().size();

        auto m2 = std::make_shared<vne::interaction::OrbitalCameraManipulator>();
        m2->setRotationMode(vne::interaction::OrbitalRotationMode::eTrackball);
        rig.addManipulator(m2);
        rig.setCamera(camera);   // re-attach camera after rebuild
        rig.onResize(kVpW, kVpH);
        VNE_LOG_INFO << "  Rebuilt with trackball: count=" << rig.manipulators().size();

        directOrbitDrag(rig, 40.0f, -20.0f);
        rig.onUpdate(kDt);
        VNE_LOG_INFO << "  Trackball rig works after rebuild";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: resetState on the rig
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: rig.resetState() ---";
    {
        auto rig = vne::interaction::CameraRig::makeOrbit();
        rig.setCamera(camera);
        rig.onResize(kVpW, kVpH);

        // Begin a drag but never send the release (simulate window focus loss)
        vne::interaction::CameraCommandPayload p;
        p.x_px = kCx; p.y_px = kCy; p.pressed = true;
        rig.onAction(vne::interaction::CameraActionType::eBeginRotate, p, kDt);

        // resetState clears the drag tracking so the next press starts cleanly
        rig.resetState();
        VNE_LOG_INFO << "  rig.resetState() called (stale drag cleared)";

        directOrbitDrag(rig, 30.0f, 10.0f);
        rig.onUpdate(kDt);
        VNE_LOG_INFO << "  Orbit works cleanly after resetState";
    }

    VNE_LOG_INFO << "07_camera_rig_composition: done.";
    return 0;
}
