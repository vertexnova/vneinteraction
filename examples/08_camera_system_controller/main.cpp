/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 08 - CameraSystemController
 * Demonstrates: using CameraSystemController as a single facade that
 * forwards all input to the active manipulator, and hot-swapping the
 * manipulator at runtime without changing the input dispatch code.
 *
 * Sequence:
 *   1. Start with OrbitManipulator, send orbit input.
 *   2. Swap to FlyManipulator at runtime, send fly input.
 *   3. Swap to OrthoPanZoomManipulator, send pan/zoom input.
 *
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/input_simulation.h"
#include "common/key_codes.h"
#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/interaction/interaction_types.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;
    using namespace vne::interaction::examples;

    const vne::interaction::CameraManipulatorFactory factory;
    vne::interaction::CameraSystemController controller;

    constexpr double dt = 1.0 / 60.0;

    // Phase 1: OrbitManipulator
    auto orbit_manip = factory.create(vne::interaction::CameraManipulatorType::eOrbit);
    controller.setManipulator(orbit_manip);
    controller.setViewportSize(1280.0f, 720.0f);

    VNE_LOG_INFO << "Phase 1: OrbitManipulator active";
    VNE_LOG_INFO << "  sceneScale=" << controller.getManipulator()->getSceneScale();

    // Helpers dispatch through the underlying manipulator; controller forwards identically
    simulateMouseDrag(*controller.getManipulator(),
                      vne::interaction::MouseButton::eLeft,
                      640.0f,
                      360.0f,
                      /*total_dx=*/60.0f,
                      /*total_dy=*/0.0f,
                      20,
                      dt);
    runFrames(*controller.getManipulator(), 20, dt);
    VNE_LOG_INFO << "After orbit drag + inertia:";
    VNE_LOG_INFO << "  worldUnitsPerPixel=" << controller.getManipulator()->getWorldUnitsPerPixel();

    // Phase 2: Swap to FlyManipulator — same controller, new manipulator
    auto fly_manip = factory.create(vne::interaction::CameraManipulatorType::eFly);
    controller.setManipulator(fly_manip);
    controller.setViewportSize(1280.0f, 720.0f);

    VNE_LOG_INFO << "Phase 2: FlyManipulator swapped in";

    simulateKeyHold(*controller.getManipulator(), key_w, 30, dt);
    VNE_LOG_INFO << "After 30 frames flying forward:";
    VNE_LOG_INFO << "  sceneScale=" << controller.getManipulator()->getSceneScale();

    // Phase 3: Swap to OrthoPanZoomManipulator
    auto ortho_manip = factory.create(vne::interaction::CameraManipulatorType::eOrthoPanZoom);
    controller.setManipulator(ortho_manip);
    controller.setViewportSize(1280.0f, 720.0f);

    VNE_LOG_INFO << "Phase 3: OrthoPanZoomManipulator swapped in";

    simulateMouseScroll(*controller.getManipulator(), -1.0f, 640.0f, 360.0f, 4, dt);
    controller.update(dt);
    VNE_LOG_INFO << "After 4x scroll zoom in:";
    VNE_LOG_INFO << "  sceneScale=" << controller.getManipulator()->getSceneScale()
                 << "  worldUnitsPerPixel=" << controller.getManipulator()->getWorldUnitsPerPixel();

    // Touch-pan (no helper — demonstrates direct controller API)
    {
        vne::interaction::TouchPan pan{10.0f, 0.0f};
        for (int i = 0; i < 10; ++i) {
            controller.handleTouchPan(pan, dt);
        }
    }
    VNE_LOG_INFO << "After touch-pan";

    // Phase 4: Detach — controller becomes a safe no-op
    controller.setManipulator(nullptr);
    controller.handleMouseButton(0, true, 640.0f, 360.0f, dt);  // safe no-op
    controller.update(dt);
    VNE_LOG_INFO << "Phase 4: Manipulator detached — all input is safely ignored";

    return 0;
}
