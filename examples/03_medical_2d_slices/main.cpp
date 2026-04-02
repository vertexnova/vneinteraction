/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 03: Medical 2D slices — Ortho2DController
 *
 * Demonstrates:
 *   - Orthographic camera setup
 *   - Default pan (LMB drag) and zoom-at-cursor (scroll)
 *   - Enable in-plane rotation for slice reorientation
 *   - Pan inertia enable/disable and damping
 *   - Zoom sensitivity tuning
 *   - Button rebinding (MMB pan, RMB rotate)
 *   - ZoomMethod variants via ortho2DManipulator() escape hatch
 *   - fitToAABB to frame a DICOM slice region
 *   - reset()
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_2d_controller.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

static constexpr double kDt = 1.0 / 60.0;
static constexpr float kVpW = 512.0f;
static constexpr float kVpH = 512.0f;
static constexpr float kCx = kVpW / 2.0f;
static constexpr float kCy = kVpH / 2.0f;

static void runUpdate(vne::interaction::Ortho2DController& ctrl,
                      std::shared_ptr<vne::scene::ICamera> camera,
                      int frames,
                      const char* label) {
    for (int i = 0; i < frames; ++i) {
        ctrl.onUpdate(kDt);
    }
    const auto pos = camera->getPosition();
    VNE_LOG_INFO << label << " — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
}

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // ── Orthographic camera (required for Ortho2DController) ─────────────────
    // Half-extents of 10 world units in each axis; near/far span z-depth.
    auto camera = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, -100.0f, 100.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 0.0f, 1.0f));
    camera->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));

    // ── Controller ────────────────────────────────────────────────────────────
    vne::interaction::Ortho2DController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(kVpW, kVpH);

    auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: Default pan + zoom (no rotation)
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: Default pan (LMB drag) + scroll zoom ---";
    VNE_LOG_INFO << "  rotation_enabled=" << ctrl.isRotationEnabled();  // false by default

    // Pan: LMB drag
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  -60.0f,
                                                  40.0f,
                                                  20,
                                                  kDt);
    runUpdate(ctrl, camera, 20, "After pan drag");

    // Zoom at cursor: scroll-in 4 ticks
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 4, kDt);
    runUpdate(ctrl, camera, 10, "After scroll zoom-in");

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: Pan inertia and damping
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: Pan inertia + damping ---";
    ctrl.setPanInertiaEnabled(true);
    // setPanSensitivity delegates to Ortho2DManipulator::setPanDamping (higher = faster stop)
    ctrl.setPanSensitivity(12.0f);

    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  80.0f,
                                                  0.0f,
                                                  15,
                                                  kDt);
    // Let inertia coast 30 frames then settle
    runUpdate(ctrl, camera, 30, "After pan release (inertia coasting)");

    ctrl.setPanInertiaEnabled(false);
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  80.0f,
                                                  0.0f,
                                                  15,
                                                  kDt);
    runUpdate(ctrl, camera, 10, "Inertia disabled — immediate stop");

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: Zoom sensitivity
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: Zoom sensitivity ---";
    ctrl.setZoomSensitivity(1.5f);  // more aggressive zoom per scroll tick
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "High zoom sensitivity");

    ctrl.setZoomSensitivity(1.05f);  // subtle zoom
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "Low zoom sensitivity");

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: Enable in-plane rotation (slice reorientation)
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: In-plane rotation enabled ---";
    ctrl.setRotationEnabled(true);
    ctrl.setRotateSensitivity(0.5f);  // degrees per pixel
    VNE_LOG_INFO << "  rotation_enabled=" << ctrl.isRotationEnabled();

    // RMB drag — rotates the view in-plane (default rotate button is RMB)
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eRight,
                                                  kCx,
                                                  kCy,
                                                  50.0f,
                                                  0.0f,
                                                  20,
                                                  kDt);
    runUpdate(ctrl, camera, 15, "After in-plane rotation drag");

    ctrl.setRotationEnabled(false);

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: Button rebinding
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: Button rebinding (MMB=pan, RMB=rotate) ---";
    ctrl.setPanButton(vne::events::MouseButton::eMiddle);
    ctrl.setRotateButton(vne::events::MouseButton::eRight);
    ctrl.setRotationEnabled(true);

    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eMiddle,
                                                  kCx,
                                                  kCy,
                                                  30.0f,
                                                  -20.0f,
                                                  12,
                                                  kDt);
    runUpdate(ctrl, camera, 10, "MMB pan after rebind");

    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eRight,
                                                  kCx,
                                                  kCy,
                                                  40.0f,
                                                  0.0f,
                                                  10,
                                                  kDt);
    runUpdate(ctrl, camera, 10, "RMB rotate after rebind");

    // Restore defaults
    ctrl.setPanButton(vne::events::MouseButton::eLeft);
    ctrl.setRotationEnabled(false);

    // ─────────────────────────────────────────────────────────────────────────
    // Section F: ZoomMethod variants via ortho2DManipulator() escape hatch
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- F: ZoomMethod variants ---";
    auto& manip = ctrl.ortho2DManipulator();

    manip.setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "ZoomMethod::eSceneScale");
    VNE_LOG_INFO << "  zoom_scale=" << manip.getZoomScale();

    manip.setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "ZoomMethod::eChangeFov (adjusts ortho half-extents)");

    manip.setZoomMethod(vne::interaction::ZoomMethod::eDollyToCoi);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "ZoomMethod::eDollyToCoi (cursor-anchored ortho zoom)");

    // ─────────────────────────────────────────────────────────────────────────
    // Section G: fitToAABB — frame a DICOM slice region
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- G: fitToAABB to DICOM region ---";
    ctrl.fitToAABB(vne::math::Vec3f(-128.0f, -128.0f, -1.0f), vne::math::Vec3f(128.0f, 128.0f, 1.0f));
    runUpdate(ctrl, camera, 60, "After fitToAABB 256×256 mm slice");

    // ─────────────────────────────────────────────────────────────────────────
    // Section H: DOF gating
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- H: DOF gating ---";
    ctrl.setZoomEnabled(false);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "Zoom disabled — scroll ignored");
    ctrl.setZoomEnabled(true);

    ctrl.setPanEnabled(false);
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  50.0f,
                                                  0.0f,
                                                  10,
                                                  kDt);
    runUpdate(ctrl, camera, 5, "Pan disabled — LMB drag ignored");
    ctrl.setPanEnabled(true);

    // ─────────────────────────────────────────────────────────────────────────
    // Section I: reset
    // ─────────────────────────────────────────────────────────────────────────
    ctrl.reset();
    runUpdate(ctrl, camera, 5, "After reset()");

    VNE_LOG_INFO << "03_medical_2d_slices: done.";
    return 0;
}
