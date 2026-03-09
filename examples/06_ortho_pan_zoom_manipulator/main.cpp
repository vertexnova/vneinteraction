/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 06 - OrthoPanZoom Manipulator
 * Demonstrates: creating an OrthoPanZoomManipulator (orthographic-only),
 * configuring zoom/pan settings, simulating middle-mouse pan, scroll
 * zoom, touch-pinch zoom, and fitToAABB for 2D/top-down views.
 *
 * No window/GPU required — all interaction is driven programmatically.
 * ----------------------------------------------------------------------
 */

#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"

#include <vertexnova/math/core/core.h>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // --- Create via factory ---
    const vne::interaction::CameraManipulatorFactory factory;
    auto manipulator = factory.create(vne::interaction::CameraManipulatorType::eOrthoPanZoom);

    auto* ortho = dynamic_cast<vne::interaction::OrthoPanZoomManipulator*>(manipulator.get());

    // --- Configure ---
    manipulator->setViewportSize(1280.0f, 720.0f);
    ortho->setZoomSpeed(1.1f);
    ortho->setPanDamping(10.0f);

    VNE_LOG_INFO << "OrthoPanZoomManipulator created";
    VNE_LOG_INFO << "  supportsPerspective=" << (manipulator->supportsPerspective() ? "yes" : "no")
                 << "  supportsOrthographic=" << (manipulator->supportsOrthographic() ? "yes" : "no");
    VNE_LOG_INFO << "  zoomSpeed=" << ortho->getZoomSpeed() << "  panDamping=" << ortho->getPanDamping()
                 << "  sceneScale=" << manipulator->getSceneScale();

    constexpr double dt = 1.0 / 60.0;

    // --- Simulate: middle-mouse pan ---
    manipulator->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eMiddle),
                                   /*pressed=*/true,
                                   640.0f,
                                   360.0f,
                                   dt);
    float mx = 640.0f;
    for (int i = 0; i < 30; ++i) {
        const float dx = 4.0f;
        manipulator->handleMouseMove(mx + dx, 360.0f, dx, 0.0f, dt);
        mx += dx;
    }
    manipulator->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eMiddle),
                                   /*pressed=*/false,
                                   mx,
                                   360.0f,
                                   dt);

    // Decay pan inertia
    for (int i = 0; i < 30; ++i) {
        manipulator->update(dt);
    }
    VNE_LOG_INFO << "After middle-mouse pan + inertia decay";
    VNE_LOG_INFO << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: scroll zoom in (negative scroll_y = zoom in) ---
    for (int i = 0; i < 5; ++i) {
        manipulator->handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, dt);
    }
    VNE_LOG_INFO << "After 5x scroll zoom in:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    // --- Simulate: scroll zoom out ---
    for (int i = 0; i < 3; ++i) {
        manipulator->handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, dt);
    }
    VNE_LOG_INFO << "After 3x scroll zoom out:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale();

    // --- Simulate: touch-pinch zoom in (scale < 1 = zoom in) ---
    {
        vne::interaction::TouchPinch pinch;
        pinch.scale = 0.85f;
        pinch.center_x_px = 640.0f;
        pinch.center_y_px = 360.0f;
        for (int i = 0; i < 5; ++i) {
            manipulator->handleTouchPinch(pinch, dt);
        }
    }
    VNE_LOG_INFO << "After 5x touch-pinch zoom in:";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale();

    // --- Simulate: touch-pan ---
    {
        vne::interaction::TouchPan pan;
        pan.delta_x_px = 5.0f;
        pan.delta_y_px = 0.0f;
        for (int i = 0; i < 20; ++i) {
            manipulator->handleTouchPan(pan, dt);
            manipulator->update(dt);
        }
    }
    VNE_LOG_INFO << "After touch-pan";

    // --- fitToAABB (2D rect in XY plane) ---
    manipulator->fitToAABB({-5.0f, -5.0f, 0.0f}, {5.0f, 5.0f, 0.0f});
    VNE_LOG_INFO << "After fitToAABB([-5,-5,0] to [5,5,0]):";
    VNE_LOG_INFO << "  sceneScale=" << manipulator->getSceneScale()
                 << "  worldUnitsPerPixel=" << manipulator->getWorldUnitsPerPixel();

    manipulator->resetState();
    VNE_LOG_INFO << "Reset complete";

    return 0;
}
