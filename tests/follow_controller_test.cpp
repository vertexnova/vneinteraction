/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/follow_controller.h"
#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <vertexnova/math/core/constants.h>
#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

TEST(FollowController, SetTargetStatic) {
    vne::interaction::FollowController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    vne::math::Mat4f target = vne::math::Mat4f::translate(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    ctrl.setTarget(target);
    ctrl.setLag(0.1f);

    ctrl.onUpdate(0.1);
    const auto pos = cam->getPosition();
    EXPECT_GT((pos - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.01f);
}

TEST(FollowController, SetTargetCallback) {
    vne::interaction::FollowController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    vne::math::Vec3f dynamic_pos(0.0f, 0.0f, 0.0f);
    ctrl.setTarget([&dynamic_pos]() { return vne::math::Mat4f::translate(dynamic_pos); });
    dynamic_pos = vne::math::Vec3f(5.0f, 0.0f, 0.0f);
    ctrl.onUpdate(0.1);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.01f);
}

TEST(FollowController, SetOffset) {
    vne::interaction::FollowController ctrl;
    ctrl.setOffset(vne::math::Vec3f(0.0f, 1.0f, 2.0f));
    const auto off = ctrl.getOffset();
    EXPECT_FLOAT_EQ(off.y(), 1.0f);
    EXPECT_FLOAT_EQ(off.z(), 2.0f);
}

TEST(FollowController, ViewDirectionAntiParallelToOffset) {
    vne::interaction::FollowController ctrl;
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, -1.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);
    ctrl.setTarget(vne::math::Mat4f::translate(vne::math::Vec3f(100.0f, 0.0f, 0.0f)));
    ctrl.setOffset(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    ctrl.followManipulator().setDamping(200.0f);

    for (int i = 0; i < 80; ++i) {
        ctrl.onUpdate(0.05);
    }

    const vne::math::Vec3f target(100.0f, 0.0f, 0.0f);
    const vne::math::Vec3f eye = cam->getPosition();
    const vne::math::Vec3f to_target = (target - eye).normalized();
    const vne::math::Vec3f off = ctrl.getOffset().normalized();
    EXPECT_NEAR(to_target.dot(off), -1.0f, 0.05f);

    const vne::math::Vec3f desired = target + ctrl.getOffset();
    EXPECT_NEAR((eye - desired).length(), 0.0f, 0.15f);
}

TEST(FollowController, OffsetWorldSpaceWithRotatedTargetTransform) {
    vne::interaction::FollowController ctrl;
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, -1.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    const vne::math::Vec3f world_offset(0.0f, 0.0f, 5.0f);
    // Non-identity rotation; translation column is still (100,0,0) for translate * rotate (world T then local R).
    const vne::math::Mat4f target =
        vne::math::Mat4f::translate(100.0f, 0.0f, 0.0f) * vne::math::Mat4f::rotateY(vne::math::kHalfPi);
    const vne::math::Vec3f target_translation(target[3][0], target[3][1], target[3][2]);

    ctrl.setTarget(target);
    ctrl.setOffset(world_offset);
    ctrl.followManipulator().setDamping(200.0f);

    for (int i = 0; i < 80; ++i) {
        ctrl.onUpdate(0.05);
    }

    const vne::math::Vec3f desired = target_translation + ctrl.getOffset();
    const vne::math::Vec3f eye = cam->getPosition();
    EXPECT_NEAR((eye - desired).length(), 0.0f, 0.15f)
        << "FollowController must apply offset in world space (target_translation + offset), not rotated by the "
           "target matrix upper-left 3x3";

    // If offset were wrongly transformed by target rotation (+90° Y), (0,0,5) would map toward +X and this point
    // would differ materially from desired.
    const vne::math::Vec3f wrong_if_local_offset_applied =
        target_translation + vne::math::Vec3f(5.0f, 0.0f, 0.0f);  // +Z_world -> +X_world under 90° Y
    EXPECT_GT((eye - wrong_if_local_offset_applied).length(), 0.5f);
}

}  // namespace vne_interaction_test
