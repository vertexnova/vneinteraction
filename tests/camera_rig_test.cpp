/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/orbit_arcball_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(CameraRig, MakeOrbit) {
    auto rig = vne::interaction::CameraRig::makeOrbit();
    EXPECT_NE(rig.behaviors().size(), 0u);
}

TEST(CameraRig, Make2D) {
    auto rig = vne::interaction::CameraRig::make2D();
    EXPECT_NE(rig.behaviors().size(), 0u);
}

TEST(CameraRig, AddRemoveBehavior) {
    vne::interaction::CameraRig rig;
    auto b = std::make_shared<vne::interaction::OrbitArcballBehavior>();
    rig.addBehavior(b);
    EXPECT_EQ(rig.behaviors().size(), 1u);
    rig.clearBehaviors();
    EXPECT_EQ(rig.behaviors().size(), 0u);
}

TEST(CameraRig, OnActionDispatchesToBehaviors) {
    auto rig = vne::interaction::CameraRig::makeOrbit();
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    rig.setCamera(cam);
    rig.setViewportSize(1280.0f, 720.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.delta_x_px = 50.0f;

    rig.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
    rig.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    rig.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.016);
    rig.onUpdate(0.016);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.01f);
}

}  // namespace vne_interaction_test
