/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/arcball.h"
#include "vertexnova/interaction/orbit_arcball_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>
#include <memory>

namespace vne_interaction_test {

TEST(Arcball, HyperbolicCenterIsUnitAndFrontHemisphere) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    ASSERT_EQ(a.projectionMode(), vne::interaction::Arcball::ProjectionMode::eHyperbolic);
    const auto v = a.project(vne::math::Vec2f(400.0f, 300.0f));
    EXPECT_NEAR(v.length(), 1.0f, 1e-5f);
    EXPECT_GT(v.z(), 0.0f);
    EXPECT_NEAR(v.x(), 0.0f, 1e-6f);
    EXPECT_NEAR(v.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(v.z(), 1.0f, 1e-5f);
}

TEST(Arcball, RimCenterMatchesHyperbolicAtOrigin) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    a.setProjectionMode(vne::interaction::Arcball::ProjectionMode::eRim);
    const auto v = a.project(vne::math::Vec2f(400.0f, 300.0f));
    EXPECT_NEAR(v.length(), 1.0f, 1e-5f);
    EXPECT_GT(v.z(), 0.0f);
}

TEST(OrbitArcballBehavior, ArcballProjectionModeForward) {
    vne::interaction::OrbitArcballBehavior b;
    EXPECT_EQ(b.getArcballProjectionMode(), vne::interaction::Arcball::ProjectionMode::eHyperbolic);
    b.setArcballProjectionMode(vne::interaction::Arcball::ProjectionMode::eRim);
    EXPECT_EQ(b.getArcballProjectionMode(), vne::interaction::Arcball::ProjectionMode::eRim);
}

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(OrbitArcballBehavior, DefaultValues) {
    vne::interaction::OrbitArcballBehavior b;
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitRotationMode::eOrbit);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eCoi);
    EXPECT_GT(b.getOrbitDistance(), 0.0f);
    EXPECT_GT(b.getZoomSpeed(), 0.0f);
}

TEST(OrbitArcballBehavior, SetRotationMode) {
    vne::interaction::OrbitArcballBehavior b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eArcball);
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitRotationMode::eArcball);
}

TEST(OrbitArcballBehavior, SetPivotMode) {
    vne::interaction::OrbitArcballBehavior b;
    b.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
}

TEST(OrbitArcballBehavior, SetOrbitDistanceClamped) {
    vne::interaction::OrbitArcballBehavior b;
    b.setOrbitDistance(0.001f);
    EXPECT_GE(b.getOrbitDistance(), 0.01f);
}

TEST(OrbitArcballBehavior, CameraIntegration) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitArcballBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.delta_x_px = 50.0f;
    p.delta_y_px = 0.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.016);
    b.onUpdate(0.016);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.01f);
}

TEST(OrbitArcballBehavior, FitToAABB) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitArcballBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    b.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));

    const auto coi = b.getCenterOfInterestWorld();
    EXPECT_NEAR(coi.x(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.y(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.z(), 0.0f, 1e-3f);
}

TEST(OrbitArcballBehavior, ResetState) {
    vne::interaction::OrbitArcballBehavior b;
    EXPECT_NO_FATAL_FAILURE(b.resetState());
}

// kFovMaxDeg in CameraBehaviorBase is 120 — must match OrbitArcballBehavior::onZoomDolly clamp.
// Use max FOV so 120 * fov_zoom_speed_ clamps back to 120 with exact float equality (new_fov == fov),
// deterministically exercising fallthrough to dolly. At min FOV, tiny float drift can make
// new_fov != fov and return early before dolly.
TEST(OrbitArcballBehavior, ChangeFovZoomFallsThroughToDollyWhenFovClamped) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitArcballBehavior b;
    b.setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    b.setFovZoomSpeed(1.05f);
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    // Pin FOV at kFovMaxDeg so zoom-out multiplies past 120° and clamps to 120° unchanged → dolly.
    constexpr float kFovMaxDeg = 120.0f;
    cam->setFieldOfView(kFovMaxDeg);
    cam->updateMatrices();

    const float orbit_before = b.getOrbitDistance();

    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.zoom_factor = 1.1f;  // zoom out (> 1): 120*1.05 clamps to 120 → fallthrough to dolly
    b.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, 0.0);

    EXPECT_FLOAT_EQ(cam->getFieldOfView(), kFovMaxDeg);
    EXPECT_GT(b.getOrbitDistance(), orbit_before)
        << "At max FOV, eChangeFov should fall through to dolly (orbit_distance changes)";
}

}  // namespace vne_interaction_test
