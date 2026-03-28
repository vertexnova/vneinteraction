/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/orbit_trackball_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>

namespace vne_interaction_test {

TEST(OrbitTrackballBehavior, SetTrackballProjectionMode) {
    vne::interaction::OrbitTrackballBehavior b;
    EXPECT_EQ(b.getTrackballProjectionMode(), vne::interaction::TrackballBehavior::ProjectionMode::eHyperbolic);
    b.setTrackballProjectionMode(vne::interaction::TrackballBehavior::ProjectionMode::eRim);
    EXPECT_EQ(b.getTrackballProjectionMode(), vne::interaction::TrackballBehavior::ProjectionMode::eRim);
}

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(OrbitTrackballBehavior, DefaultValues) {
    vne::interaction::OrbitTrackballBehavior b;
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitRotationMode::eOrbit);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eCoi);
    EXPECT_GT(b.getOrbitDistance(), 0.0f);
    EXPECT_GT(b.getZoomSpeed(), 0.0f);
}

TEST(OrbitTrackballBehavior, SetRotationMode) {
    vne::interaction::OrbitTrackballBehavior b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eArcball);
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitRotationMode::eArcball);
}

TEST(OrbitTrackballBehavior, SetArcballRotationScale) {
    vne::interaction::OrbitTrackballBehavior b;
    EXPECT_FLOAT_EQ(b.getTrackballRotationScale(), 2.5f);
    b.setTrackballRotationScale(1.0f);
    EXPECT_FLOAT_EQ(b.getTrackballRotationScale(), 1.0f);
}

TEST(OrbitTrackballBehavior, SetPivotMode) {
    vne::interaction::OrbitTrackballBehavior b;
    b.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
}

/** Guard against accidental `OrbitPivotMode` enumerator reorder (ABI / persisted values). */
TEST(OrbitTrackballBehavior, OrbitPivotModeUnderlyingValues) {
    using vne::interaction::OrbitPivotMode;
    EXPECT_EQ(static_cast<std::uint8_t>(OrbitPivotMode::eCoi), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(OrbitPivotMode::eViewCenter), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(OrbitPivotMode::eFixed), 2u);
}

TEST(OrbitTrackballBehavior, SetOrbitDistanceClamped) {
    vne::interaction::OrbitTrackballBehavior b;
    b.setOrbitDistance(0.001f);
    EXPECT_GE(b.getOrbitDistance(), 0.01f);
}

TEST(OrbitTrackballBehavior, CameraIntegration) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitTrackballBehavior b;
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

TEST(OrbitTrackballBehavior, FitToAABB) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitTrackballBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    b.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));

    const auto coi = b.getCenterOfInterestWorld();
    EXPECT_NEAR(coi.x(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.y(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.z(), 0.0f, 1e-3f);
}

TEST(OrbitTrackballBehavior, ResetState) {
    vne::interaction::OrbitTrackballBehavior b;
    EXPECT_NO_FATAL_FAILURE(b.resetState());
}

// kFovMaxDeg in CameraBehaviorBase is 120 — must match OrbitTrackballBehavior::onZoomDolly clamp.
// Use max FOV so 120 * fov_zoom_speed_ clamps back to 120 with exact float equality (new_fov == fov),
// deterministically exercising fallthrough to dolly. At min FOV, tiny float drift can make
// new_fov != fov and return early before dolly.
TEST(OrbitTrackballBehavior, ChangeFovZoomFallsThroughToDollyWhenFovClamped) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitTrackballBehavior b;
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

// ---------------------------------------------------------------------------
// eArcball rotation + inertia (integration; math details in trackball_behavior_test.cpp)
// ---------------------------------------------------------------------------

[[nodiscard]] static float arcballInertiaStepMagnitude(float end_x_px) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitTrackballBehavior b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eArcball);
    b.setCamera(cam);
    b.onResize(800.0f, 600.0f);

    constexpr double kDt = 0.016;
    vne::interaction::CameraCommandPayload p;
    p.x_px = 400.0f;
    p.y_px = 300.0f;
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, kDt);

    p.x_px = end_x_px;
    p.y_px = 300.0f;
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, kDt);

    const vne::math::Vec3f pos_after_drag = cam->getPosition();
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, kDt);
    b.onUpdate(kDt);

    return (cam->getPosition() - pos_after_drag).length();
}

TEST(OrbitTrackballBehavior, ArcballInertiaMovesCameraAfterRotateEnds) {
    EXPECT_GT(arcballInertiaStepMagnitude(620.0f), 1e-4f);
}

TEST(OrbitTrackballBehavior, ArcballInertiaNotUpdatedWhenDeltaTimeBelowInertiaThreshold) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitTrackballBehavior b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eArcball);
    b.setCamera(cam);
    b.onResize(800.0f, 600.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 400.0f;
    p.y_px = 300.0f;
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);

    p.x_px = 620.0f;
    p.y_px = 300.0f;
    // OrbitTrackballBehavior: inertia sampling requires delta_time >= kMinDeltaTimeForInertia (0.001).
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 1e-5);

    const vne::math::Vec3f pos_after_drag = cam->getPosition();
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 1e-5);
    b.onUpdate(0.016);

    EXPECT_LT((cam->getPosition() - pos_after_drag).length(), 1e-3f);
}

TEST(OrbitTrackballBehavior, ArcballLargeDragProducesStrongerInertiaThanSmallDrag) {
    const float small_step = arcballInertiaStepMagnitude(430.0f);
    const float large_step = arcballInertiaStepMagnitude(650.0f);
    EXPECT_GT(large_step, small_step);
}

}  // namespace vne_interaction_test
