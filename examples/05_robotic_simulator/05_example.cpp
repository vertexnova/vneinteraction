#include "05_example.h"

#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

#include <cmath>

static constexpr double kDt = 1.0 / 60.0;
static constexpr float kVpW = 1280.0f;
static constexpr float kVpH = 720.0f;
static constexpr float kCx = kVpW / 2.0f;
static constexpr float kCy = kVpH / 2.0f;

// Simulated robot end-effector position (circular path, radius 2, height 1.5)
static vne::math::Vec3f simulatedEndEffector(double t) {
    return {static_cast<float>(2.0 * std::cos(t)), 1.5f, static_cast<float>(2.0 * std::sin(t))};
}

namespace vne::interaction::examples {

int runRoboticSimulatorExample() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // ── Shared perspective camera ─────────────────────────────────────────────
    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(50.0f, kVpW / kVpH, 0.01f, 500.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 3.0f, 8.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 1.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // ── Two controllers sharing the same camera ───────────────────────────────
    vne::interaction::Inspect3DController inspect;
    vne::interaction::Navigation3DController navigate;

    for (auto* c : {static_cast<vne::interaction::ICameraController*>(&inspect),
                    static_cast<vne::interaction::ICameraController*>(&navigate)}) {
        c->setCamera(camera);
        c->onResize(kVpW, kVpH);
    }

    // ── Inspect: orbit around robot base, fixed pivot ─────────────────────────
    inspect.setPivot(vne::math::Vec3f(0.0f, 0.5f, 0.0f));
    inspect.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
    inspect.setRotationInertiaEnabled(true);
    inspect.orbitalCameraManipulator().setRotationDamping(5.0f);

    // ── Navigate: FPS, reasonable speed for a workshop floor ─────────────────
    navigate.setMode(vne::interaction::FreeLookMode::eFps);
    navigate.setMoveSpeed(3.0f);
    navigate.setSprintMultiplier(5.0f);

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: Inspect mode — orbit robot arm
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: Inspect mode (orbit robot arm) ---";
    {
        auto on_event = [&](const vne::events::Event& e, double dt) { inspect.onEvent(e, dt); };

        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eLeft,
                                                      kCx,
                                                      kCy,
                                                      120.0f,
                                                      30.0f,
                                                      25,
                                                      kDt);
        for (int i = 0; i < 40; ++i)
            inspect.onUpdate(kDt);

        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  After inspect orbit — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";

        // Fit to the arm's bounding box
        inspect.fitToAABB(vne::math::Vec3f(-1.0f, 0.0f, -1.0f), vne::math::Vec3f(1.0f, 2.0f, 1.0f));
        for (int i = 0; i < 60; ++i)
            inspect.onUpdate(kDt);
        VNE_LOG_INFO << "  After fitToAABB";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: Switch → Navigate (Tab pattern)
    //   On switch: reset() clears stale drag/key state from the previous controller.
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: Switch to Navigate (FPS walk) ---";
    inspect.reset();   // clear orbit inertia velocity before handing off
    navigate.reset();  // sync angles from current camera pose

    {
        auto on_event = [&](const vne::events::Event& e, double dt) { navigate.onEvent(e, dt); };

        // Hold RMB to look, walk forward
        vne::events::MouseButtonPressedEvent rmb(vne::events::MouseButton::eRight,
                                                 0,
                                                 static_cast<double>(kCx),
                                                 static_cast<double>(kCy));
        on_event(rmb, kDt);
        vne::events::MouseMovedEvent look(static_cast<double>(kCx + 20.0f), static_cast<double>(kCy - 10.0f));
        on_event(look, kDt);

        vne::interaction::examples::simulateKeyHold(on_event, vne::events::KeyCode::eW, 30, kDt, [&](double dt) {
            navigate.onUpdate(dt);
        });

        vne::events::MouseButtonReleasedEvent rmb_rel(vne::events::MouseButton::eRight,
                                                      0,
                                                      static_cast<double>(kCx + 20.0f),
                                                      static_cast<double>(kCy - 10.0f));
        on_event(rmb_rel, kDt);

        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  After FPS walk — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: Inspect — chase moving end-effector by updating pivot each frame
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: Inspect with moving pivot (effector path) ---";
    navigate.reset();
    inspect.reset();
    inspect.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
    inspect.orbitalCameraManipulator().setRotationDamping(8.0f);

    double sim_time = 0.0;
    for (int i = 0; i < 120; ++i) {
        sim_time += kDt;
        inspect.setPivot(simulatedEndEffector(sim_time));
        inspect.onUpdate(kDt);
    }
    {
        const auto pos = camera->getPosition();
        const auto eff = simulatedEndEffector(sim_time);
        VNE_LOG_INFO << "  effector=(" << eff.x() << "," << eff.y() << "," << eff.z() << ")"
                     << "  eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: Rotation damping — snappy vs floaty orbit decay
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: Orbit rotation damping ---";

    inspect.orbitalCameraManipulator().setRotationDamping(20.0f);
    inspect.setPivot(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    {
        auto on_event = [&](const vne::events::Event& e, double dt) { inspect.onEvent(e, dt); };
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eLeft,
                                                      kCx,
                                                      kCy,
                                                      100.0f,
                                                      0.0f,
                                                      12,
                                                      kDt);
    }
    for (int i = 0; i < 25; ++i)
        inspect.onUpdate(kDt);
    {
        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  High damping (20): eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    inspect.orbitalCameraManipulator().setRotationDamping(2.0f);
    {
        auto on_event = [&](const vne::events::Event& e, double dt) { inspect.onEvent(e, dt); };
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eLeft,
                                                      kCx,
                                                      kCy,
                                                      -80.0f,
                                                      0.0f,
                                                      10,
                                                      kDt);
    }
    for (int i = 0; i < 35; ++i)
        inspect.onUpdate(kDt);
    {
        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  Low damping (2): eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: Static pivot (robot base)
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: Static pivot (robot base) ---";
    inspect.setPivot(vne::math::Vec3f(0.0f, 0.5f, 0.0f));
    inspect.orbitalCameraManipulator().setRotationDamping(6.0f);
    for (int i = 0; i < 40; ++i)
        inspect.onUpdate(kDt);
    {
        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  Static pivot: eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section F: Switch back to inspect — reset first
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- F: More inspect orbit ---";
    inspect.reset();

    {
        auto on_event = [&](const vne::events::Event& e, double dt) { inspect.onEvent(e, dt); };
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eLeft,
                                                      kCx,
                                                      kCy,
                                                      -60.0f,
                                                      20.0f,
                                                      15,
                                                      kDt);
        for (int i = 0; i < 20; ++i)
            inspect.onUpdate(kDt);
        VNE_LOG_INFO << "  Inspect orbit after reset";
    }

    VNE_LOG_INFO << "05_robotic_simulator: done.";
    return 0;
}

}  // namespace vne::interaction::examples
