/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(Navigation3DController, DefaultFpsMode) {
    vne::interaction::Navigation3DController ctrl;
    EXPECT_EQ(ctrl.getMode(), vne::interaction::NavigateMode::eFps);
}

TEST(Navigation3DController, SetMode) {
    vne::interaction::Navigation3DController ctrl;
    ctrl.setMode(vne::interaction::NavigateMode::eFly);
    EXPECT_EQ(ctrl.getMode(), vne::interaction::NavigateMode::eFly);
    ctrl.setMode(vne::interaction::NavigateMode::eGame);
    EXPECT_EQ(ctrl.getMode(), vne::interaction::NavigateMode::eGame);
}

TEST(Navigation3DController, OrbitTrackballBehaviorNullInFpsMode) {
    vne::interaction::Navigation3DController ctrl;
    ctrl.setMode(vne::interaction::NavigateMode::eFps);
    EXPECT_EQ(ctrl.orbitTrackballBehavior(), nullptr);
}

TEST(Navigation3DController, OrbitTrackballBehaviorNonNullInGameMode) {
    vne::interaction::Navigation3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);
    ctrl.setMode(vne::interaction::NavigateMode::eGame);
    EXPECT_NE(ctrl.orbitTrackballBehavior(), nullptr);
}

TEST(Navigation3DController, OnEventNoCrash) {
    vne::interaction::Navigation3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    vne::events::KeyPressedEvent press(vne::events::KeyCode::eW);
    vne::events::MouseButtonPressedEvent look(vne::events::MouseButton::eRight, 0, 640.0, 360.0);

    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(press, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(look, 0.016));
}

}  // namespace vne_interaction_test
