/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

/**
 * Regression tests for manipulator fixes: pan direction, pitch direction,
 * zoom-to-cursor, mouseToNDC, and buildReferenceFrame / trackball pole safety.
 */

#include "vertexnova/interaction/manipulator_utils.h"
#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <gtest/gtest.h>
#include <memory>

namespace vne_interaction_regression_test {

// ---------------------------------------------------------------------------
// mouseToNDC
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, MouseToNDC_Corners) {
    const float w = 800.0f;
    const float h = 600.0f;
    vne::math::Vec2f ndc;

    ndc = vne::interaction::mouseToNDC(0.0f, 0.0f, w, h);
    EXPECT_NEAR(ndc.x(), -1.0f, 1e-5f);
    EXPECT_NEAR(ndc.y(), 1.0f, 1e-5f);  // top-left -> NDC (+1, +1) for Y-up

    ndc = vne::interaction::mouseToNDC(w, 0.0f, w, h);
    EXPECT_NEAR(ndc.x(), 1.0f, 1e-5f);
    EXPECT_NEAR(ndc.y(), 1.0f, 1e-5f);

    ndc = vne::interaction::mouseToNDC(0.0f, h, w, h);
    EXPECT_NEAR(ndc.x(), -1.0f, 1e-5f);
    EXPECT_NEAR(ndc.y(), -1.0f, 1e-5f);

    ndc = vne::interaction::mouseToNDC(w * 0.5f, h * 0.5f, w, h);
    EXPECT_NEAR(ndc.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(ndc.y(), 0.0f, 1e-5f);
}

TEST(ManipulatorRegression, MouseToNDC_InvalidViewport) {
    auto ndc = vne::interaction::mouseToNDC(100.0f, 100.0f, 0.0f, 600.0f);
    EXPECT_NEAR(ndc.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(ndc.y(), 0.0f, 1e-5f);
}

// ---------------------------------------------------------------------------
// buildReferenceFrame: ref_right is actual RIGHT
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, BuildReferenceFrame_RightVector) {
    vne::math::Vec3f world_up(0.0f, 1.0f, 0.0f);
    vne::math::Vec3f ref_fwd, ref_right;
    vne::interaction::buildReferenceFrame(world_up, ref_fwd, ref_right);
    EXPECT_NEAR(ref_fwd.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(ref_fwd.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(ref_fwd.z(), -1.0f, 1e-5f);
    EXPECT_NEAR(ref_right.x(), 1.0f, 1e-5f);
    EXPECT_NEAR(ref_right.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(ref_right.z(), 0.0f, 1e-5f);
}

// ---------------------------------------------------------------------------
// Ortho pan direction: drag right -> scene moves right (camera/target move left)
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, OrthoPan_DragRightMovesSceneRight) {
    auto ortho = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    ortho->lookAt(vne::math::Vec3f(0.0f, 0.0f, 10.0f),
                  vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                  vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ortho->updateMatrices();

    vne::interaction::Ortho2DManipulator b;
    b.setCamera(ortho);
    b.onResize(200.0f, 200.0f);

    vne::math::Vec3f target_before = ortho->getTarget();
    vne::interaction::CameraCommandPayload p;
    p.delta_x_px = 20.0f;
    p.delta_y_px = 0.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginPan, p, 0.0);
    b.onAction(vne::interaction::CameraActionType::ePanDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndPan, p, 0.0);

    vne::math::Vec3f target_after = ortho->getTarget();
    float delta_x_world = target_after.x() - target_before.x();
    EXPECT_LT(delta_x_world, 0.0f)
        << "Drag right (drag-the-scene) should move camera/target left (negative X) so scene appears to move right";
}

TEST(ManipulatorRegression, OrthoPan_DragDownMovesSceneDown) {
    auto ortho = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    ortho->lookAt(vne::math::Vec3f(0.0f, 0.0f, 10.0f),
                  vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                  vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ortho->updateMatrices();

    vne::interaction::Ortho2DManipulator b;
    b.setCamera(ortho);
    b.onResize(200.0f, 200.0f);

    vne::math::Vec3f target_before = ortho->getTarget();
    vne::interaction::CameraCommandPayload p;
    p.delta_x_px = 0.0f;
    p.delta_y_px = 20.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginPan, p, 0.0);
    b.onAction(vne::interaction::CameraActionType::ePanDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndPan, p, 0.0);

    vne::math::Vec3f target_after = ortho->getTarget();
    float delta_y_world = target_after.y() - target_before.y();
    EXPECT_GT(delta_y_world, 0.0f)
        << "Drag down should move camera/target along +up so scene appears to move down with the cursor";
}

// ---------------------------------------------------------------------------
// Orbital pan (perspective + ortho): vertical drag matches Ortho2D sign (COI moves +camera up on screen-down)
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, OrbitPan_Perspective_DragDownMovesCoiAlongUp) {
    auto persp = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    persp->lookAt(vne::math::Vec3f(0.0f, 0.0f, 10.0f),
                  vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                  vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    persp->updateMatrices();

    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eOrbit);
    b.setCamera(persp);
    b.onResize(200.0f, 200.0f);

    const vne::math::Vec3f target_before = persp->getTarget();
    vne::interaction::CameraCommandPayload p;
    p.x_px = 100.0f;
    p.y_px = 100.0f;
    p.delta_x_px = 0.0f;
    p.delta_y_px = 20.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginPan, p, 0.0);
    b.onAction(vne::interaction::CameraActionType::ePanDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndPan, p, 0.0);

    const float delta_y_world = persp->getTarget().y() - target_before.y();
    EXPECT_GT(delta_y_world, 0.0f)
        << "Orbit pan (perspective): drag down should shift COI along +view up (positive dY for default look)";
}

TEST(ManipulatorRegression, OrbitPan_Orthographic_DragDownMovesCoiAlongUp) {
    auto ortho = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    ortho->lookAt(vne::math::Vec3f(0.0f, 0.0f, 10.0f),
                  vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                  vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ortho->updateMatrices();

    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eOrbit);
    b.setCamera(ortho);
    b.onResize(200.0f, 200.0f);

    const vne::math::Vec3f target_before = ortho->getTarget();
    vne::interaction::CameraCommandPayload p;
    p.x_px = 100.0f;
    p.y_px = 100.0f;
    p.delta_x_px = 0.0f;
    p.delta_y_px = 20.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginPan, p, 0.0);
    b.onAction(vne::interaction::CameraActionType::ePanDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndPan, p, 0.0);

    const float delta_y_world = ortho->getTarget().y() - target_before.y();
    EXPECT_GT(delta_y_world, 0.0f)
        << "Orbit pan (orthographic): drag down should shift COI along +view up (positive dY for default look)";
}

// ---------------------------------------------------------------------------
// FreeLook pitch: mouse up (negative delta_y) -> look up (pitch increases)
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, FreeLook_MouseUpLooksUp) {
    auto persp = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    persp->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    persp->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::FreeLookManipulator b;
    b.setCamera(persp);
    b.onResize(800.0f, 600.0f);

    b.onAction(vne::interaction::CameraActionType::eBeginLook, vne::interaction::CameraCommandPayload{}, 0.0);

    vne::math::Vec3f target_before = persp->getTarget();
    vne::math::Vec3f pos_before = persp->getPosition();
    vne::math::Vec3f fwd_before = (target_before - pos_before).normalized();

    vne::interaction::CameraCommandPayload p;
    p.delta_x_px = 0.0f;
    p.delta_y_px = -30.0f;  // mouse up
    b.onAction(vne::interaction::CameraActionType::eLookDelta, p, 0.016);

    vne::math::Vec3f fwd_after = (persp->getTarget() - persp->getPosition()).normalized();
    EXPECT_GT(fwd_after.y(), fwd_before.y()) << "Mouse up should look up (increase forward Y)";
}

// ---------------------------------------------------------------------------
// Zoom-to-cursor (ortho): point under cursor stays fixed after zoom
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, OrthoZoomToCursor_KeepsPointUnderCursor) {
    auto ortho = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 1000.0f));
    ortho->lookAt(vne::math::Vec3f(0.0f, 0.0f, 10.0f),
                  vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                  vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ortho->updateMatrices();

    vne::interaction::Ortho2DManipulator b;
    b.setCamera(ortho);
    const float w = 100.0f;
    const float h = 100.0f;
    b.onResize(w, h);

    float cursor_x = w * 0.5f;
    float cursor_y = h * 0.5f;
    vne::interaction::CameraCommandPayload p;
    p.x_px = cursor_x;
    p.y_px = cursor_y;
    p.zoom_factor = 0.5f;

    vne::math::Vec2f ndc = vne::interaction::mouseToNDC(cursor_x, cursor_y, w, h);
    vne::math::Vec3f world_before = vne::interaction::worldUnderCursorOrtho(*ortho, ndc.x(), ndc.y());

    b.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, 0.0);

    vne::math::Vec3f world_after = vne::interaction::worldUnderCursorOrtho(*ortho, ndc.x(), ndc.y());
    float dist = (world_after - world_before).length();
    EXPECT_LT(dist, 0.01f) << "World point under cursor should stay fixed after zoom";
}

// ---------------------------------------------------------------------------
// Orbit Euler: positive yaw = turn right
// ---------------------------------------------------------------------------
TEST(ManipulatorRegression, OrbitEuler_PositiveYawTurnsRight) {
    auto persp = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    persp->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    persp->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eOrbit);
    b.setCamera(persp);
    b.onResize(800.0f, 600.0f);

    vne::math::Vec3f pos_before = persp->getPosition();
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, vne::interaction::CameraCommandPayload{}, 0.0);
    vne::interaction::CameraCommandPayload p;
    p.delta_x_px = 50.0f;
    p.delta_y_px = 0.0f;
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.0);

    vne::math::Vec3f pos_after = persp->getPosition();
    EXPECT_LT(pos_after.x(), pos_before.x())
        << "Drag right (positive delta_x) should orbit camera left (position X decreases in RH Y-up)";
}

// Trackball mode uses absolute cursor in eRotateDelta; same horizontal drag should match Euler orbit sense.
TEST(ManipulatorRegression, OrbitalCamera_HorizontalDragMatchesEulerSign) {
    auto persp = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    persp->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    persp->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setRotationMode(vne::interaction::OrbitRotationMode::eTrackball);
    b.setCamera(persp);
    b.onResize(800.0f, 600.0f);

    vne::math::Vec3f pos_before = persp->getPosition();
    vne::interaction::CameraCommandPayload p;
    p.x_px = 400.0f;
    p.y_px = 300.0f;
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
    p.x_px = 450.0f;
    p.y_px = 300.0f;
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.0);

    vne::math::Vec3f pos_after = persp->getPosition();
    EXPECT_LT(pos_after.x(), pos_before.x()) << "Trackball drag right should match Euler: camera X decreases";
}

}  // namespace vne_interaction_regression_test
