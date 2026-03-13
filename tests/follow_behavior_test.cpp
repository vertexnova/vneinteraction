/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/follow_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(FollowBehavior, SetTargetWorld) {
    vne::interaction::FollowBehavior b;
    b.setTargetWorld(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    const auto t = b.getTargetWorld();
    EXPECT_FLOAT_EQ(t.x(), 1.0f);
    EXPECT_FLOAT_EQ(t.y(), 2.0f);
    EXPECT_FLOAT_EQ(t.z(), 3.0f);
}

TEST(FollowBehavior, SetTargetProvider) {
    vne::interaction::FollowBehavior b;
    b.setTargetProvider([]() { return vne::math::Vec3f(5.0f, 0.0f, 0.0f); });
    const auto t = b.getTargetWorld();
    EXPECT_FLOAT_EQ(t.x(), 5.0f);
}

TEST(FollowBehavior, CameraMovesTowardTarget) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::FollowBehavior b;
    b.setCamera(cam);
    b.setViewportSize(1280.0f, 720.0f);
    b.setTargetWorld(vne::math::Vec3f(100.0f, 0.0f, 0.0f));
    b.setOffset(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    b.setDamping(50.0f);

    const auto pos_before = cam->getPosition();
    b.onUpdate(0.1);
    const auto pos_after = cam->getPosition();

    EXPECT_GT((pos_after - pos_before).length(), 0.1f);
}

}  // namespace vne_interaction_test
