/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/ortho_2d_controller.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
}

TEST(Ortho2DController, DefaultRotationDisabled) {
    vne::interaction::Ortho2DController ctrl;
    EXPECT_FALSE(ctrl.isRotationEnabled());
}

TEST(Ortho2DController, SetRotationEnabled) {
    vne::interaction::Ortho2DController ctrl;
    ctrl.setRotationEnabled(true);
    EXPECT_TRUE(ctrl.isRotationEnabled());
}

TEST(Ortho2DController, FitToAABB) {
    vne::interaction::Ortho2DController ctrl;
    auto cam = makeOrthoCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(512.0f, 512.0f);

    EXPECT_NO_FATAL_FAILURE(ctrl.fitToAABB(vne::math::Vec3f(-2.0f, -2.0f, 0.0f), vne::math::Vec3f(2.0f, 2.0f, 0.0f)));
}

TEST(Ortho2DController, OnEventNoCrash) {
    vne::interaction::Ortho2DController ctrl;
    auto cam = makeOrthoCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(512.0f, 512.0f);

    vne::events::MouseMovedEvent move(256.0, 256.0);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);

    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(move));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(scroll));
}

TEST(Ortho2DController, DisablePanOrRotationMidGestureEndsManipulatorLatch) {
    constexpr double kDt = 0.016;
    auto cam = makeOrthoCamera();

    {
        vne::interaction::Ortho2DController ctrl;
        ctrl.setCamera(cam);
        ctrl.onResize(512.0f, 512.0f);
        ctrl.onEvent(vne::events::MouseMovedEvent(256.0, 256.0), kDt);
        ctrl.onEvent(
            vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eLeft, 0, 256.0, 256.0), kDt);
        ctrl.onEvent(vne::events::MouseMovedEvent(300.0, 260.0), kDt);
        EXPECT_NO_FATAL_FAILURE(ctrl.setPanEnabled(false));
        for (int i = 0; i < 5; ++i) {
            ctrl.onUpdate(kDt);
        }
        EXPECT_NO_FATAL_FAILURE(ctrl.setPanEnabled(true));
        ctrl.onEvent(
            vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eLeft, 0, 300.0, 260.0), kDt);
    }

    {
        vne::interaction::Ortho2DController ctrl;
        ctrl.setCamera(cam);
        ctrl.onResize(512.0f, 512.0f);
        ctrl.setRotationEnabled(true);
        ctrl.onEvent(vne::events::MouseMovedEvent(256.0, 256.0), kDt);
        ctrl.onEvent(
            vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eRight, 0, 256.0, 256.0), kDt);
        ctrl.onEvent(vne::events::MouseMovedEvent(280.0, 240.0), kDt);
        EXPECT_NO_FATAL_FAILURE(ctrl.setRotationEnabled(false));
        for (int i = 0; i < 5; ++i) {
            ctrl.onUpdate(kDt);
        }
        EXPECT_NO_FATAL_FAILURE(ctrl.setRotationEnabled(true));
        ctrl.onEvent(
            vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eRight, 0, 280.0, 240.0), kDt);
    }
}

}  // namespace vne_interaction_test
