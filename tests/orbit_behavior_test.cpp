/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/orbit_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>
#include <memory>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(OrbitBehavior, DefaultValues) {
    vne::interaction::OrbitBehavior b;
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitRotationMode::eEuler);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eCoi);
    EXPECT_GT(b.getOrbitDistance(), 0.0f);
    EXPECT_GT(b.getZoomSpeed(), 0.0f);
}

TEST(OrbitBehavior, SetRotationMode) {
    vne::interaction::OrbitBehavior b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eQuaternion);
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitRotationMode::eQuaternion);
}

TEST(OrbitBehavior, SetPivotMode) {
    vne::interaction::OrbitBehavior b;
    b.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
}

TEST(OrbitBehavior, SetOrbitDistanceClamped) {
    vne::interaction::OrbitBehavior b;
    b.setOrbitDistance(0.001f);
    EXPECT_GE(b.getOrbitDistance(), 0.01f);
}

TEST(OrbitBehavior, CameraIntegration) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitBehavior b;
    b.setCamera(cam);
    b.setViewportSize(1280.0f, 720.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.delta_x_px = 50.0f;
    p.delta_y_px = 0.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.016);
    b.update(0.016);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.01f);
}

TEST(OrbitBehavior, FitToAABB) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitBehavior b;
    b.setCamera(cam);
    b.setViewportSize(1280.0f, 720.0f);

    b.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));

    const auto coi = b.getCenterOfInterestWorld();
    EXPECT_NEAR(coi.x(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.y(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.z(), 0.0f, 1e-3f);
}

TEST(OrbitBehavior, ResetState) {
    vne::interaction::OrbitBehavior b;
    EXPECT_NO_FATAL_FAILURE(b.resetState());
}

}  // namespace vne_interaction_test
