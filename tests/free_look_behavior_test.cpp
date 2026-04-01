/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <vertexnova/math/core/core.h>

#include <cmath>

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

static std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(vne::scene::OrthographicCameraParameters(-10.0f,
                                                                                                  10.0f,
                                                                                                  -10.0f * 9.0f / 16.0f,
                                                                                                  10.0f * 9.0f / 16.0f,
                                                                                                  0.1f,
                                                                                                  1000.0f));
}

TEST(FreeLookBehavior, DefaultFpsMode) {
    vne::interaction::FreeLookBehavior b;
    EXPECT_EQ(b.getMode(), vne::interaction::FreeLookMode::eFps);
    EXPECT_TRUE(b.getConstrainWorldUp());
}

TEST(FreeLookBehavior, SetMode) {
    vne::interaction::FreeLookBehavior b;
    b.setMode(vne::interaction::FreeLookMode::eFly);
    EXPECT_EQ(b.getMode(), vne::interaction::FreeLookMode::eFly);
    EXPECT_FALSE(b.getConstrainWorldUp());
}

TEST(FreeLookBehavior, SetConstrainWorldUp) {
    vne::interaction::FreeLookBehavior b;
    b.setConstrainWorldUp(false);
    EXPECT_EQ(b.getMode(), vne::interaction::FreeLookMode::eFly);
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
    b.onResize(1280.0f, 720.0f);
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

TEST(FreeLookBehavior, OrthoMoveForwardPansInImagePlane) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::FreeLookBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setMoveSpeed(5.0f);

    const float y0 = cam->getPosition().y();

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);

    EXPECT_GT(std::abs(cam->getPosition().y() - y0), 1e-4f);
}

TEST(FreeLookBehavior, KeyboardMoveCompensatesSceneScale) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::FreeLookBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setMoveSpeed(10.0f);

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);

    const float step_unit_scale = (cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 10.0f)).length();
    EXPECT_NEAR(step_unit_scale, 1.0f, 1e-4f);

    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->setSceneScale(2.0f);
    b.setCamera(cam);

    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);

    const float step_double_scale = (cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 10.0f)).length();
    // View bakes scale(s,s,1); halve world translation so image-plane motion stays comparable.
    EXPECT_NEAR(step_double_scale, 0.5f, 1e-4f);
}

TEST(FreeLookBehavior, ResetStateResyncsAnglesFromCamera) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::FreeLookBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setMoveSpeed(5.0f);

    // Simulate external camera reset without setCamera (e.g. testbed resetCamera).
    cam->setPosition(vne::math::Vec3f(-10.0f, 0.0f, 0.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();

    b.resetState();

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);

    // Forward must follow the new +X view toward the origin, not the stale -Z basis.
    EXPECT_GT(cam->getPosition().x(), -10.0f);
}

/**
 * Handoff after external camera pose change: markAnglesDirty() sets angles_dirty_; onUpdate runs
 * ensureAnglesSynced() (which calls syncAnglesFromCamera) before WASD so yaw_deg_/pitch_deg_ match the rig.
 * Subcase A: perspCamera() path uses view forward for move_forward; view_offset (target - position) preserved.
 */
TEST(FreeLookBehavior, ExternalPoseHandoff_PerspectiveForwardAndViewOffset) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::FreeLookBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setMoveSpeed(5.0f);

    // External orbit-style retarget without setCamera: eye moves, view axis changes.
    cam->setPosition(vne::math::Vec3f(-10.0f, 0.0f, 0.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();
    b.markAnglesDirty();

    const vne::math::Vec3f view_before = cam->getTarget() - cam->getPosition();
    const float view_len_before = view_before.length();
    ASSERT_GT(view_len_before, 1e-3f);
    const vne::math::Vec3f view_dir_before = view_before / view_len_before;

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);

    EXPECT_GT(cam->getPosition().x(), -10.0f);
    const vne::math::Vec3f view_after = cam->getTarget() - cam->getPosition();
    const float view_len_after = view_after.length();
    EXPECT_NEAR(view_len_after, view_len_before, 1e-3f);
    const vne::math::Vec3f view_dir_after = view_after / view_len_after;
    EXPECT_NEAR(view_dir_before.dot(view_dir_after), 1.0f, 1e-3f);
}

/**
 * Zero-dt onUpdate must still run ensureAnglesSynced / syncAnglesFromCamera so the next positive-dt move
 * does not use stale yaw_deg_/pitch_deg_ after markAnglesDirty().
 */
TEST(FreeLookBehavior, OnUpdateZeroDeltaTimeStillSyncsAnglesFromCamera) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::FreeLookBehavior b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setMoveSpeed(5.0f);

    cam->setPosition(vne::math::Vec3f(-10.0f, 0.0f, 0.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();
    b.markAnglesDirty();

    b.onUpdate(0.0);
    b.onUpdate(-1.0);

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveForward, p, 0.016);

    EXPECT_GT(cam->getPosition().x(), -10.0f);
}

/**
 * Fly + orthographic: orthoPanUp(view_dir, vertical_axis) builds image-plane forward; input_state_.move_up
 * moves along roll-aware vertical_axis (upVector()). After markAnglesDirty and external lookAt with non-world up,
 * move_up should track camera up, not world +Y only.
 */
TEST(FreeLookBehavior, ExternalPoseHandoff_OrthoFlyMoveUpAlongCameraUp) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(1.0f, 0.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::FreeLookBehavior b;
    b.setMode(vne::interaction::FreeLookMode::eFly);
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setMoveSpeed(8.0f);

    cam->setPosition(vne::math::Vec3f(2.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(2.0f, 0.0f, 0.0f), vne::math::Vec3f(1.0f, 0.0f, 0.0f));
    cam->updateMatrices();
    b.markAnglesDirty();

    const vne::math::Vec3f pos_before_move = cam->getPosition();
    const vne::math::Vec3f view_before = cam->getTarget() - cam->getPosition();

    vne::interaction::CameraCommandPayload p;
    p.pressed = true;
    b.onAction(vne::interaction::CameraActionType::eMoveUp, p, 0.016);
    b.onUpdate(0.1);
    p.pressed = false;
    b.onAction(vne::interaction::CameraActionType::eMoveUp, p, 0.016);

    const vne::math::Vec3f delta = cam->getPosition() - pos_before_move;
    // Camera "up" is +X; vertical motion should be primarily along X, not world +Y.
    EXPECT_GT(std::abs(delta.x()), 0.05f);
    EXPECT_LT(std::abs(delta.y()), 0.05f);

    const vne::math::Vec3f view_after = cam->getTarget() - cam->getPosition();
    EXPECT_NEAR(view_before.length(), view_after.length(), 1e-3f);
    const vne::math::Vec3f vb = view_before / view_before.length();
    const vne::math::Vec3f va = view_after / view_after.length();
    EXPECT_NEAR(vb.dot(va), 1.0f, 1e-3f);
}

}  // namespace vne_interaction_test
