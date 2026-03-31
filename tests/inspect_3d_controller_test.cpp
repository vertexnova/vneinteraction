/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

namespace {

constexpr double kEventDt = 0.016;

/** LMB press at (x0,y0), drag to (x1,y1), release — matches window event order used by dispatchMouseEvents. */
void simulateLeftButtonHorizontalDrag(
    vne::interaction::Inspect3DController& ctrl, double x0, double y0, double x1, double y1) {
    ctrl.onEvent(vne::events::MouseMovedEvent(x0, y0), kEventDt);
    ctrl.onEvent(vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eLeft, 0, x0, y0), kEventDt);
    ctrl.onEvent(vne::events::MouseMovedEvent(x1, y1), kEventDt);
    ctrl.onEvent(vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eLeft, 0, x1, y1), kEventDt);
    ctrl.onUpdate(kEventDt);
}

}  // namespace

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(Inspect3DController, DefaultOrbitRotationMode) {
    vne::interaction::Inspect3DController ctrl;
    EXPECT_EQ(ctrl.getRotationMode(), vne::interaction::OrbitRotationMode::eOrbit);
    EXPECT_TRUE(ctrl.isRotationEnabled());
}

TEST(Inspect3DController, SetPivotMode) {
    vne::interaction::Inspect3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    ctrl.setPivot(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(ctrl.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
}

TEST(Inspect3DController, PivotOnDoubleClickIndependentOfRotation) {
    vne::interaction::Inspect3DController ctrl;
    EXPECT_TRUE(ctrl.isPivotOnDoubleClickEnabled());
    ctrl.setRotationEnabled(false);
    EXPECT_FALSE(ctrl.isRotationEnabled());

    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    ctrl.setPivot(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(ctrl.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);

    vne::events::MouseButtonDoubleClickedEvent dbl(vne::events::MouseButton::eLeft, 0, 640.0, 360.0);
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(dbl, kEventDt));
    EXPECT_EQ(ctrl.getPivotMode(), vne::interaction::OrbitPivotMode::eCoi)
        << "eSetPivotAtCursor should fire with rotation off when pivot-on-double-click is enabled";

    ctrl.setPivot(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(ctrl.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
    ctrl.setPivotOnDoubleClickEnabled(false);
    EXPECT_FALSE(ctrl.isPivotOnDoubleClickEnabled());
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(dbl, kEventDt));
    EXPECT_EQ(ctrl.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed)
        << "Double-click should not change pivot mode when pivot-on-double-click is disabled";
}

TEST(Inspect3DController, SetRotationEnabled) {
    vne::interaction::Inspect3DController ctrl;
    EXPECT_EQ(ctrl.getRotationMode(), vne::interaction::OrbitRotationMode::eOrbit);
    EXPECT_TRUE(ctrl.isRotationEnabled());

    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    ctrl.setRotationEnabled(false);
    EXPECT_FALSE(ctrl.isRotationEnabled());

    const vne::math::Vec3f pos_rotation_off = cam->getPosition();
    simulateLeftButtonHorizontalDrag(ctrl, 640.0, 360.0, 690.0, 360.0);
    EXPECT_LT((cam->getPosition() - pos_rotation_off).length(), 1e-5f)
        << "LMB drag must not affect the camera while rotation is disabled";

    ctrl.setRotationEnabled(true);
    EXPECT_TRUE(ctrl.isRotationEnabled());

    const vne::math::Vec3f pos_before_orbit = cam->getPosition();
    simulateLeftButtonHorizontalDrag(ctrl, 640.0, 360.0, 690.0, 360.0);
    EXPECT_GT((cam->getPosition() - pos_before_orbit).length(), 1e-4f)
        << "Same LMB drag should orbit the camera once rotation is enabled";

    ctrl.setRotationMode(vne::interaction::OrbitRotationMode::eTrackball);
    EXPECT_EQ(ctrl.getRotationMode(), vne::interaction::OrbitRotationMode::eTrackball);
}

TEST(Inspect3DController, FitToAABB) {
    vne::interaction::Inspect3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    EXPECT_NO_FATAL_FAILURE(ctrl.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f)));
}

TEST(Inspect3DController, OnEventNoCrash) {
    vne::interaction::Inspect3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    vne::events::MouseMovedEvent move(640.0, 360.0);
    vne::events::MouseButtonPressedEvent press(vne::events::MouseButton::eLeft, 0, 640.0, 360.0);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);

    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(move, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(press, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(scroll, 0.016));
}

}  // namespace vne_interaction_test
