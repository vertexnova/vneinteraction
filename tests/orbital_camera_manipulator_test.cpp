/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>

namespace vne_interaction_test {

TEST(OrbitalCameraManipulator, SetTrackballProjectionMode) {
    vne::interaction::OrbitalCameraManipulator b;
    EXPECT_EQ(b.getTrackballProjectionMode(), vne::interaction::TrackballProjectionMode::eHyperbolic);
    b.setTrackballProjectionMode(vne::interaction::TrackballProjectionMode::eRim);
    EXPECT_EQ(b.getTrackballProjectionMode(), vne::interaction::TrackballProjectionMode::eRim);
}

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(OrbitalCameraManipulator, DefaultValues) {
    vne::interaction::OrbitalCameraManipulator b;
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitalRotationMode::eTrackball);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eCoi);
    EXPECT_GT(b.getOrbitDistance(), 0.0f);
    EXPECT_GT(b.getZoomSpeed(), 0.0f);
}

TEST(OrbitalCameraManipulator, SetRotationMode) {
    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitalRotationMode::eTrackball);
    EXPECT_EQ(b.getRotationMode(), vne::interaction::OrbitalRotationMode::eTrackball);
}

TEST(OrbitalCameraManipulator, SetTrackballRotationScale) {
    vne::interaction::OrbitalCameraManipulator b;
    EXPECT_FLOAT_EQ(b.getTrackballRotationScale(), 2.5f);
    b.setTrackballRotationScale(1.0f);
    EXPECT_FLOAT_EQ(b.getTrackballRotationScale(), 1.0f);
}

TEST(OrbitalCameraManipulator, SetPivotMode) {
    vne::interaction::OrbitalCameraManipulator b;
    b.setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
    EXPECT_EQ(b.getPivotMode(), vne::interaction::OrbitPivotMode::eFixed);
}

/** Guard against accidental `OrbitPivotMode` enumerator reorder (ABI / persisted values). */
TEST(OrbitalCameraManipulator, OrbitPivotModeUnderlyingValues) {
    using vne::interaction::OrbitPivotMode;
    EXPECT_EQ(static_cast<std::uint8_t>(OrbitPivotMode::eCoi), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(OrbitPivotMode::eViewCenter), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(OrbitPivotMode::eFixed), 2u);
}

TEST(OrbitalCameraManipulator, SetOrbitDistanceClamped) {
    vne::interaction::OrbitalCameraManipulator b;
    b.setOrbitDistance(0.001f);
    EXPECT_GE(b.getOrbitDistance(), 0.01f);
}

TEST(OrbitalCameraManipulator, CameraIntegration) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
    p.x_px = 690.0f;
    p.y_px = 360.0f;
    p.delta_x_px = 50.0f;
    p.delta_y_px = 0.0f;
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.016);
    b.onUpdate(0.016);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.01f);
}

TEST(OrbitalCameraManipulator, FitToAABB) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitalCameraManipulator b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    b.fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));

    const auto coi = b.getCenterOfInterestWorld();
    EXPECT_NEAR(coi.x(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.y(), 0.0f, 1e-3f);
    EXPECT_NEAR(coi.z(), 0.0f, 1e-3f);
}

TEST(OrbitalCameraManipulator, ResetState) {
    vne::interaction::OrbitalCameraManipulator b;
    EXPECT_NO_FATAL_FAILURE(b.resetState());
}

// eChangeFov uses applyFovZoom only; at FOV clamp there is no dolly fallback (use eDollyToCoi instead).
TEST(OrbitalCameraManipulator, ChangeFovAtClampDoesNotChangeOrbitDistance) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);

    constexpr float kFovMaxDeg = 160.0f;
    cam->setFieldOfView(kFovMaxDeg);
    cam->updateMatrices();

    const float orbit_before = b.getOrbitDistance();

    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.zoom_factor = 1.1f;
    b.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, 0.0);

    EXPECT_FLOAT_EQ(cam->getFieldOfView(), kFovMaxDeg);
    EXPECT_FLOAT_EQ(b.getOrbitDistance(), orbit_before);
}

// ---------------------------------------------------------------------------
// eTrackball rotation + inertia (integration; math details in trackball_behavior_test.cpp)
// ---------------------------------------------------------------------------

[[nodiscard]] static float trackballInertiaStepMagnitude(float end_x_px) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitalRotationMode::eTrackball);
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

TEST(OrbitalCameraManipulator, TrackballInertiaMovesCameraAfterRotateEnds) {
    EXPECT_GT(trackballInertiaStepMagnitude(620.0f), 1e-4f);
}

TEST(OrbitalCameraManipulator, TrackballInertiaNotUpdatedWhenDeltaTimeBelowInertiaThreshold) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitalRotationMode::eTrackball);
    b.setCamera(cam);
    b.onResize(800.0f, 600.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 400.0f;
    p.y_px = 300.0f;
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);

    p.x_px = 620.0f;
    p.y_px = 300.0f;
    // OrbitalCameraManipulator: inertia sampling requires delta_time >= kMinDeltaTimeForInertia (0.001).
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 1e-5);

    const vne::math::Vec3f pos_after_drag = cam->getPosition();
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 1e-5);
    b.onUpdate(0.016);

    EXPECT_LT((cam->getPosition() - pos_after_drag).length(), 1e-3f);
}

TEST(OrbitalCameraManipulator, TrackballLargeDragProducesStrongerInertiaThanSmallDrag) {
    const float small_step = trackballInertiaStepMagnitude(430.0f);
    const float large_step = trackballInertiaStepMagnitude(650.0f);
    EXPECT_GT(large_step, small_step);
}

}  // namespace vne_interaction_test
