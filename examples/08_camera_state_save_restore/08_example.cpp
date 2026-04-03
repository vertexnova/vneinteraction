#include "08_example.h"

#include "vertexnova/interaction/camera_state.h"
#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
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

// ── Helpers to capture and apply OrbitCameraState ────────────────────────────
// Pack live manipulator state into OrbitCameraState (Euler orbit mode).

static vne::interaction::OrbitCameraState captureOrbitState(const vne::interaction::OrbitalCameraManipulator& manip) {
    vne::interaction::OrbitCameraState s;
    s.coi_world = manip.getCenterOfInterestWorld();
    s.distance = manip.getOrbitDistance();
    s.world_up = manip.getWorldUp();
    s.yaw_deg = manip.getOrbitYawDeg();
    s.pitch_deg = manip.getOrbitPitchDeg();
    return s;
}

static void logOrbitState(const char* label, const vne::interaction::OrbitCameraState& s) {
    VNE_LOG_INFO << label << " coi=(" << s.coi_world.x() << "," << s.coi_world.y() << "," << s.coi_world.z() << ")"
                 << " dist=" << s.distance << " yaw=" << s.yaw_deg << " pitch=" << s.pitch_deg;
}

static void logFreeCameraState(const char* label, const vne::interaction::FreeCameraState& s) {
    VNE_LOG_INFO << label << " pos=(" << s.position.x() << "," << s.position.y() << "," << s.position.z() << ")"
                 << " yaw=" << s.yaw_deg << " pitch=" << s.pitch_deg;
}

namespace vne::interaction::examples {

int runCameraStateSaveRestoreExample() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: OrbitCameraState — bookmark/undo for Inspect3DController
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: OrbitCameraState bookmark / undo ---";
    {
        auto camera = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, kVpW / kVpH, 0.1f, 1000.0f));
        camera->setPosition(vne::math::Vec3f(0.0f, 0.0f, 8.0f));
        camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

        vne::interaction::Inspect3DController ctrl;
        ctrl.setCamera(camera);
        ctrl.onResize(kVpW, kVpH);

        auto& manip = ctrl.orbitalCameraManipulator();
        auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

        // ── Save bookmark A (initial pose) ───────────────────────────────────
        const vne::interaction::OrbitCameraState bookmark_a = captureOrbitState(manip);
        logOrbitState("  Bookmark A (initial):", bookmark_a);

        // ── Interact: orbit around ────────────────────────────────────────────
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eLeft,
                                                      kCx,
                                                      kCy,
                                                      150.0f,
                                                      60.0f,
                                                      30,
                                                      kDt);
        for (int i = 0; i < 30; ++i)
            ctrl.onUpdate(kDt);

        // ── Save bookmark B (after orbit) ─────────────────────────────────────
        const vne::interaction::OrbitCameraState bookmark_b = captureOrbitState(manip);
        logOrbitState("  Bookmark B (after orbit):", bookmark_b);
        VNE_LOG_INFO << "  distance_b=" << manip.getOrbitDistance() << " vs distance_a=" << bookmark_a.distance;

        // ── Pan + zoom further ────────────────────────────────────────────────
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eRight,
                                                      kCx,
                                                      kCy,
                                                      -30.0f,
                                                      10.0f,
                                                      15,
                                                      kDt);
        vne::interaction::examples::simulateMouseScroll(on_event, -1.0f, kCx, kCy, 5, kDt);
        for (int i = 0; i < 20; ++i)
            ctrl.onUpdate(kDt);
        VNE_LOG_INFO << "  Current distance=" << manip.getOrbitDistance() << " (panned + zoomed away from bookmark_b)";

        // ── Restore bookmark A — COI, distance, world up, and Euler yaw/pitch ─
        manip.setWorldUp(bookmark_a.world_up);
        manip.setPivot(bookmark_a.coi_world);
        manip.setOrbitDistance(bookmark_a.distance);
        manip.setOrbitEulerDegrees(bookmark_a.yaw_deg, bookmark_a.pitch_deg);
        for (int i = 0; i < 10; ++i)
            ctrl.onUpdate(kDt);
        VNE_LOG_INFO << "  After restore to bookmark_a: distance=" << manip.getOrbitDistance()
                     << " yaw=" << manip.getOrbitYawDeg() << " pitch=" << manip.getOrbitPitchDeg();

        // ── OrbitCameraState struct fields ───────────────────────────────────
        // Shown for documentation purposes — all fields are plain POD.
        vne::interaction::OrbitCameraState demo;
        demo.coi_world = {1.0f, 0.5f, 0.0f};
        demo.distance = 6.0f;
        demo.world_up = {0.0f, 1.0f, 0.0f};
        demo.yaw_deg = 45.0f;
        demo.pitch_deg = -20.0f;
        logOrbitState("  POD demo state:", demo);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: TrackballCameraState
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: TrackballCameraState ---";
    {
        auto camera = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, kVpW / kVpH, 0.1f, 1000.0f));
        camera->setPosition(vne::math::Vec3f(0.0f, 0.0f, 8.0f));
        camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

        vne::interaction::Inspect3DController ctrl;
        ctrl.setCamera(camera);
        ctrl.onResize(kVpW, kVpH);
        ctrl.setRotationMode(vne::interaction::OrbitalRotationMode::eTrackball);

        auto& manip = ctrl.orbitalCameraManipulator();
        auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

        // Interact
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eLeft,
                                                      kCx,
                                                      kCy,
                                                      80.0f,
                                                      40.0f,
                                                      20,
                                                      kDt);
        for (int i = 0; i < 20; ++i)
            ctrl.onUpdate(kDt);

        // Capture TrackballCameraState from live manipulator (quaternion + orbit frame)
        vne::interaction::TrackballCameraState tb_state;
        tb_state.coi_world = manip.getCenterOfInterestWorld();
        tb_state.distance = manip.getOrbitDistance();
        tb_state.world_up = manip.getWorldUp();
        tb_state.rotation = manip.getTrackballOrientation();

        VNE_LOG_INFO << "  TrackballCameraState: coi=(" << tb_state.coi_world.x() << "," << tb_state.coi_world.y()
                     << "," << tb_state.coi_world.z() << ")"
                     << " dist=" << tb_state.distance << " quat=(" << tb_state.rotation.x << "," << tb_state.rotation.y
                     << "," << tb_state.rotation.z << "," << tb_state.rotation.w << ")";

        // Perturb, then restore full bookmark (COI, distance, up, orientation)
        manip.setPivot(tb_state.coi_world + vne::math::Vec3f(2.0f, 0.0f, 0.0f));
        manip.setOrbitDistance(tb_state.distance * 1.25f);
        ctrl.onUpdate(kDt);

        manip.setWorldUp(tb_state.world_up);
        manip.setPivot(tb_state.coi_world);
        manip.setOrbitDistance(tb_state.distance);
        manip.setTrackballOrientation(tb_state.rotation);
        for (int i = 0; i < 10; ++i)
            ctrl.onUpdate(kDt);
        const auto q = manip.getTrackballOrientation();
        VNE_LOG_INFO << "  After full trackball restore: quat=(" << q.x << "," << q.y << "," << q.z << "," << q.w
                     << ")";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: FreeCameraState — bookmark for Navigation3DController
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: FreeCameraState bookmark ---";
    {
        auto camera = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(60.0f, kVpW / kVpH, 0.1f, 2000.0f));
        camera->setPosition(vne::math::Vec3f(5.0f, 2.0f, 5.0f));
        camera->lookAt(vne::math::Vec3f(0.0f, 1.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

        vne::interaction::Navigation3DController ctrl;
        ctrl.setCamera(camera);
        ctrl.onResize(kVpW, kVpH);
        ctrl.setMoveSpeed(6.0f);

        auto& manip = ctrl.freeLookManipulator();
        auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

        // ── Bookmark initial position ─────────────────────────────────────────
        vne::interaction::FreeCameraState saved;
        saved.position = camera->getPosition();
        saved.yaw_deg = 0.0f;  // internal — use camera direction in practice
        saved.pitch_deg = 0.0f;
        saved.up_hint = {0.0f, 1.0f, 0.0f};
        logFreeCameraState("  Bookmark (initial):", saved);

        // ── Walk around ───────────────────────────────────────────────────────
        vne::events::MouseButtonPressedEvent rmb(vne::events::MouseButton::eRight,
                                                 0,
                                                 static_cast<double>(kCx),
                                                 static_cast<double>(kCy));
        on_event(rmb, kDt);

        vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 40, kDt, [&](double dt) {
            ctrl.onUpdate(dt);
        });
        vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eD, 20, kDt, [&](double dt) {
            ctrl.onUpdate(dt);
        });

        vne::events::MouseButtonReleasedEvent rmb_rel(vne::events::MouseButton::eRight,
                                                      0,
                                                      static_cast<double>(kCx),
                                                      static_cast<double>(kCy));
        on_event(rmb_rel, kDt);

        // ── Capture new position ──────────────────────────────────────────────
        vne::interaction::FreeCameraState after_walk;
        after_walk.position = camera->getPosition();
        after_walk.yaw_deg = 0.0f;
        after_walk.pitch_deg = 0.0f;
        after_walk.up_hint = manip.getWorldUp();
        logFreeCameraState("  After walk:", after_walk);

        // ── Restore to saved bookmark ─────────────────────────────────────────
        // Set camera position directly, then signal manipulator to re-sync angles.
        camera->setPosition(saved.position);
        camera->setTarget(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
        camera->updateMatrices();
        manip.markAnglesDirty();  // tells FreeLookManipulator to re-derive yaw/pitch
        ctrl.reset();
        for (int i = 0; i < 5; ++i)
            ctrl.onUpdate(kDt);
        vne::interaction::FreeCameraState after_restore;
        after_restore.position = camera->getPosition();
        after_restore.yaw_deg = 0.0f;
        after_restore.pitch_deg = 0.0f;
        after_restore.up_hint = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
        logFreeCameraState("  After restore to bookmark:", after_restore);

        // ── FreeCameraState struct fields ─────────────────────────────────────
        vne::interaction::FreeCameraState demo;
        demo.position = {-3.0f, 1.5f, 7.0f};
        demo.yaw_deg = 135.0f;
        demo.pitch_deg = -10.0f;
        demo.up_hint = {0.0f, 1.0f, 0.0f};
        logFreeCameraState("  POD demo FreeCameraState:", demo);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: FreeLookInputState — held-key diagnostic snapshot
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: FreeLookInputState (held-key snapshot) ---";
    {
        // FreeLookInputState is a plain struct — you can construct it to
        // synthetically drive a FreeLookManipulator (e.g. from a gamepad axis).
        vne::interaction::FreeLookInputState state;
        state.move_forward = true;
        state.sprint = true;
        state.looking = false;
        VNE_LOG_INFO << "  FreeLookInputState: forward=" << state.move_forward << " sprint=" << state.sprint
                     << " looking=" << state.looking << " (fields document the manipulator's per-frame input model)";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: OrbitalInteractionState — drag-tracking snapshot
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: OrbitalInteractionState (drag-tracking snapshot) ---";
    {
        vne::interaction::OrbitalInteractionState state;
        state.rotating = false;
        state.panning = false;
        state.modifier_shift = false;
        state.last_x_px = 0.0f;
        state.last_y_px = 0.0f;
        VNE_LOG_INFO << "  OrbitalInteractionState: rotating=" << state.rotating << " panning=" << state.panning
                     << " (used by OrbitalCameraManipulator internally;"
                     << " shown here as documentation of the per-frame drag model)";
    }

    VNE_LOG_INFO << "08_camera_state_save_restore: done.";
    return 0;
}

}  // namespace vne::interaction::examples
