/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/inspect_controller.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(InspectController, DefaultArcballMode) {
    vne::interaction::InspectController ctrl;
    EXPECT_EQ(ctrl.getRotationMode(), vne::interaction::OrbitRotationMode::eArcball);
}

TEST(InspectController, SetPivotMode) {
    vne::interaction::InspectController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.setViewportSize(1280.0f, 720.0f);

    ctrl.setPivot(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(ctrl.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
}

TEST(InspectController, SetRotationEnabled) {
    vne::interaction::InspectController ctrl;
    ctrl.setRotationEnabled(false);
    ctrl.setRotationEnabled(true);
    EXPECT_TRUE(ctrl.getRotationMode() == vne::interaction::OrbitRotationMode::eArcball
                || ctrl.getRotationMode() == vne::interaction::OrbitRotationMode::eOrbit);
}

TEST(InspectController, FitToAABB) {
    vne::interaction::InspectController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.setViewportSize(1280.0f, 720.0f);

    EXPECT_NO_FATAL_FAILURE(ctrl.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f)));
}

TEST(InspectController, OnEventNoCrash) {
    vne::interaction::InspectController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.setViewportSize(1280.0f, 720.0f);

    vne::events::MouseMovedEvent move(640.0, 360.0);
    vne::events::MouseButtonPressedEvent press(vne::events::MouseButton::eLeft, 0, 640.0, 360.0);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);

    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(move, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(press, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(scroll, 0.016));
}

}  // namespace vne_interaction_test
