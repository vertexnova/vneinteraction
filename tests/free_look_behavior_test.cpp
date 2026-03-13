/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(FreeLookBehavior, DefaultFpsMode) {
    vne::interaction::FreeLookBehavior b;
    EXPECT_TRUE(b.getConstrainWorldUp());
}

TEST(FreeLookBehavior, SetConstrainWorldUp) {
    vne::interaction::FreeLookBehavior b;
    b.setConstrainWorldUp(false);
    EXPECT_FALSE(b.getConstrainWorldUp());
}

TEST(FreeLookBehavior, SetMoveSpeed) {
    vne::interaction::FreeLookBehavior b;
    b.setMoveSpeed(10.0f);
    EXPECT_FLOAT_EQ(b.getMoveSpeed(), 10.0f);
}

TEST(FreeLookBehavior, SetSprintMultiplier) {
    vne::interaction::FreeLookBehavior b;
    b.setSprintMultiplier(5.0f);
    EXPECT_FLOAT_EQ(b.getSprintMultiplier(), 5.0f);
}

TEST(FreeLookBehavior, CameraIntegration) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, -1.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::FreeLookBehavior b;
    b.setCamera(cam);
    b.setViewportSize(1280.0f, 720.0f);
    b.setMoveSpeed(5.0f);

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eMoveBackward, p, 0.016);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 0.0f)).length(), 0.01f);
}

}  // namespace vne_interaction_test
