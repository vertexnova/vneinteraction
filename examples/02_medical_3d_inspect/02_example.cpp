#include "02_example.h"

#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

static constexpr double kDt = 1.0 / 60.0;
static constexpr float kVpW = 1280.0f;
static constexpr float kVpH = 720.0f;
static constexpr float kCx = kVpW / 2.0f;
static constexpr float kCy = kVpH / 2.0f;

// Helper: run N update frames and log the camera position.
static void runUpdate(vne::interaction::Inspect3DController& ctrl,
                      std::shared_ptr<vne::scene::ICamera> camera,
                      int frames,
                      const char* label) {
    for (int i = 0; i < frames; ++i) {
        ctrl.onUpdate(kDt);
    }
    const auto pos = camera->getPosition();
    VNE_LOG_INFO << label << " — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
}

namespace vne::interaction::examples {

int runMedical3dInspectExample() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // ── Camera ────────────────────────────────────────────────────────────────
    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, kVpW / kVpH, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 0.0f, 8.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // ── Controller ────────────────────────────────────────────────────────────
    vne::interaction::Inspect3DController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(kVpW, kVpH);

    auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: Virtual trackball orbit, eCoi pivot, inertia on
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: Trackball orbit, eCoi pivot, inertia on ---";
    ctrl.setPivotMode(vne::interaction::OrbitPivotMode::eCoi);

    // Sensitivity
    ctrl.setRotateSensitivity(0.3f);
    ctrl.setPanSensitivity(1.0f);
    ctrl.setZoomSensitivity(1.15f);

    // Inertia: both rotation and pan coast after release
    ctrl.setRotationInertiaEnabled(true);
    ctrl.setPanInertiaEnabled(true);
    // Fine-tune damping via escape hatch (higher = quicker stop)
    auto& manip = ctrl.orbitalCameraManipulator();
    manip.setRotationDamping(6.0f);
    manip.setPanDamping(8.0f);

    VNE_LOG_INFO << "  orbit_distance=" << manip.getOrbitDistance()
                 << "  rotation_damping=" << manip.getRotationDamping() << "  pan_damping=" << manip.getPanDamping();

    // LMB drag — rotate
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  100.0f,
                                                  50.0f,
                                                  30,
                                                  kDt);
    runUpdate(ctrl, camera, 60, "Trackball orbit after LMB drag + inertia decay");

    // RMB drag — pan
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eRight,
                                                  kCx,
                                                  kCy,
                                                  -40.0f,
                                                  20.0f,
                                                  20,
                                                  kDt);
    runUpdate(ctrl, camera, 30, "After RMB pan");

    // Scroll — zoom in (negative = in by library convention)
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 4, kDt);
    runUpdate(ctrl, camera, 10, "After scroll zoom-in");

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: Trackball projection variants
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: Trackball (eHyperbolic) ---";
    manip.setTrackballProjectionMode(vne::interaction::TrackballProjectionMode::eHyperbolic);
    manip.setTrackballRotationScale(2.5f);

    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  80.0f,
                                                  -30.0f,
                                                  20,
                                                  kDt);
    runUpdate(ctrl, camera, 20, "Trackball eHyperbolic drag");

    VNE_LOG_INFO << "--- B2: Trackball (eRim) ---";
    manip.setTrackballProjectionMode(vne::interaction::TrackballProjectionMode::eRim);
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kVpW - 50.0f,
                                                  kCy,
                                                  -30.0f,
                                                  60.0f,
                                                  15,
                                                  kDt);
    runUpdate(ctrl, camera, 15, "Trackball eRim edge drag");

    manip.setTrackballProjectionMode(vne::interaction::TrackballProjectionMode::eHyperbolic);

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: Anatomy landmark — fixed pivot
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: Fixed pivot at anatomical landmark ---";
    ctrl.setPivot(vne::math::Vec3f(0.3f, 0.5f, 0.1f));            // landmark
    ctrl.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);  // pan trucks eye without moving COI
    ctrl.setPivotOnDoubleClickEnabled(false);                     // disable accidental pivot change

    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  120.0f,
                                                  0.0f,
                                                  25,
                                                  kDt);
    runUpdate(ctrl, camera, 30, "Fixed-pivot orbit");

    const auto coi = manip.getCenterOfInterestWorld();
    VNE_LOG_INFO << "  COI=" << coi.x() << "," << coi.y() << "," << coi.z() << "  (should remain near landmark)";

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: DOF toggles
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: DOF toggles ---";
    // Disable rotation: LMB drag no longer orbits
    ctrl.setRotationEnabled(false);
    VNE_LOG_INFO << "  rotation_enabled=" << ctrl.isRotationEnabled();
    vne::interaction::examples::simulateMouseDrag(on_event,
                                                  vne::events::MouseButton::eLeft,
                                                  kCx,
                                                  kCy,
                                                  100.0f,
                                                  0.0f,
                                                  10,
                                                  kDt);
    runUpdate(ctrl, camera, 5, "Rotation disabled — no orbit from LMB");

    ctrl.setRotationEnabled(true);
    ctrl.setPanEnabled(false);
    VNE_LOG_INFO << "  pan disabled; zoom still active";
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "Pan disabled — zoom still works");
    ctrl.setPanEnabled(true);

    ctrl.setZoomEnabled(false);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "Zoom disabled — scroll ignored");
    ctrl.setZoomEnabled(true);

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: ZoomMethod variants (via escape hatch)
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: ZoomMethod variants ---";

    // eSceneScale: virtual zoom, no geometry movement
    manip.setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "ZoomMethod::eSceneScale");
    VNE_LOG_INFO << "  zoom_scale=" << manip.getZoomScale();

    // eChangeFov: shrinks/enlarges FOV
    manip.setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "ZoomMethod::eChangeFov");

    // eDollyToCoi: moves camera along view ray (typical orbit zoom)
    manip.setZoomMethod(vne::interaction::ZoomMethod::eDollyToCoi);
    vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 3, kDt);
    runUpdate(ctrl, camera, 5, "ZoomMethod::eDollyToCoi");

    // ─────────────────────────────────────────────────────────────────────────
    // Section F: fitToAABB + view direction presets
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- F: fitToAABB + view presets ---";
    ctrl.fitToAABB(vne::math::Vec3f(-1.5f, -1.5f, -1.5f), vne::math::Vec3f(1.5f, 1.5f, 1.5f));
    runUpdate(ctrl, camera, 60, "After fitToAABB (60 frames of smooth anim)");

    manip.setViewDirection(vne::interaction::ViewDirection::eTop);
    runUpdate(ctrl, camera, 10, "View preset: eTop");

    manip.setViewDirection(vne::interaction::ViewDirection::eFront);
    runUpdate(ctrl, camera, 10, "View preset: eFront");

    manip.setViewDirection(vne::interaction::ViewDirection::eIso);
    runUpdate(ctrl, camera, 10, "View preset: eIso");

    // ─────────────────────────────────────────────────────────────────────────
    // Section G: Interaction speed step keys (optional runtime rebind)
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- G: Interaction speed step keys ---";
    ctrl.setIncreaseInteractionSpeedKey(vne::events::KeyCode::eRightBracket);  // ]
    ctrl.setDecreaseInteractionSpeedKey(vne::events::KeyCode::eLeftBracket);   // [
    ctrl.setInteractionSpeedStep(1.2f);  // each press multiplies/divides sensitivity by 1.2

    // Simulate pressing ] three times → speed grows by 1.2^3
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eRightBracket, 1, kDt, [&](double dt) {
        ctrl.onUpdate(dt);
    });
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eRightBracket, 1, kDt, [&](double dt) {
        ctrl.onUpdate(dt);
    });
    vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eRightBracket, 1, kDt, [&](double dt) {
        ctrl.onUpdate(dt);
    });
    VNE_LOG_INFO << "  After 3× ] presses: rotation_speed=" << manip.getRotationSpeed()
                 << "  pan_speed=" << manip.getPanSpeed();

    // ─────────────────────────────────────────────────────────────────────────
    // Section H: reset
    // ─────────────────────────────────────────────────────────────────────────
    ctrl.reset();
    runUpdate(ctrl, camera, 5, "After reset()");

    VNE_LOG_INFO << "02_medical_3d_inspect: done.";
    return 0;
}

}  // namespace vne::interaction::examples
