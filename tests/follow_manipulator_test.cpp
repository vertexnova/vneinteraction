/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class FollowManipulatorTest : public testing::Test {
   protected:
    void SetUp() override {
        manip_ = std::make_unique<vne::interaction::FollowManipulator>();
        camera_ = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
        camera_->setViewport(1280.0f, 720.0f);
    }

    std::unique_ptr<vne::interaction::FollowManipulator> manip_;
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
};

// --- Interface capability ---

TEST_F(FollowManipulatorTest, SupportsPerspective) {
    EXPECT_TRUE(manip_->supportsPerspective());
}

TEST_F(FollowManipulatorTest, SupportsOrthographic) {
    EXPECT_TRUE(manip_->supportsOrthographic());
}

// --- Default values ---

TEST_F(FollowManipulatorTest, DefaultSceneScaleIsOne) {
    EXPECT_FLOAT_EQ(manip_->getSceneScale(), 1.0f);
}

TEST_F(FollowManipulatorTest, DefaultDamping) {
    EXPECT_FLOAT_EQ(manip_->getDamping(), 8.0f);
}

TEST_F(FollowManipulatorTest, DefaultZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.1f);
}

TEST_F(FollowManipulatorTest, DefaultZoomMethodIsDollyToCoi) {
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eDollyToCoi);
}

TEST_F(FollowManipulatorTest, DefaultOffsetWorldIsAlongZ) {
    const auto& offset = manip_->getOffsetWorld();
    EXPECT_FLOAT_EQ(offset.z(), 5.0f);
}

// --- Setters and getters ---

TEST_F(FollowManipulatorTest, SetTargetWorldStoresValue) {
    vne::math::Vec3f target{1.0f, 2.0f, 3.0f};
    manip_->setTargetWorld(target);
    const auto stored = manip_->getTargetWorld();
    EXPECT_FLOAT_EQ(stored.x(), 1.0f);
    EXPECT_FLOAT_EQ(stored.y(), 2.0f);
    EXPECT_FLOAT_EQ(stored.z(), 3.0f);
}

TEST_F(FollowManipulatorTest, SetOffsetWorldStoresValue) {
    vne::math::Vec3f offset{0.0f, 5.0f, -10.0f};
    manip_->setOffsetWorld(offset);
    const auto& stored = manip_->getOffsetWorld();
    EXPECT_FLOAT_EQ(stored.x(), 0.0f);
    EXPECT_FLOAT_EQ(stored.y(), 5.0f);
    EXPECT_FLOAT_EQ(stored.z(), -10.0f);
}

TEST_F(FollowManipulatorTest, SetDampingStoresValue) {
    manip_->setDamping(5.0f);
    EXPECT_FLOAT_EQ(manip_->getDamping(), 5.0f);
}

TEST_F(FollowManipulatorTest, SetDampingClampsToZero) {
    manip_->setDamping(-2.0f);
    EXPECT_FLOAT_EQ(manip_->getDamping(), 0.0f);
}

TEST_F(FollowManipulatorTest, SetZoomSpeedStoresValue) {
    manip_->setZoomSpeed(1.3f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.3f);
}

TEST_F(FollowManipulatorTest, SetZoomSpeedClampsToMinimum) {
    manip_->setZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.01f);
}

TEST_F(FollowManipulatorTest, SetZoomMethodSceneScale) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eSceneScale);
}

TEST_F(FollowManipulatorTest, SetTargetProviderOverridesStaticTarget) {
    vne::math::Vec3f dynamic_target{10.0f, 20.0f, 30.0f};
    manip_->setTargetProvider([&dynamic_target]() { return dynamic_target; });
    const auto result = manip_->getTargetWorld();
    EXPECT_FLOAT_EQ(result.x(), 10.0f);
    EXPECT_FLOAT_EQ(result.y(), 20.0f);
    EXPECT_FLOAT_EQ(result.z(), 30.0f);
}

TEST_F(FollowManipulatorTest, SetTargetProviderNullptrFallsBackToStaticTarget) {
    vne::math::Vec3f static_target{5.0f, 6.0f, 7.0f};
    manip_->setTargetWorld(static_target);
    manip_->setTargetProvider(nullptr);
    const auto result = manip_->getTargetWorld();
    EXPECT_FLOAT_EQ(result.x(), 5.0f);
    EXPECT_FLOAT_EQ(result.y(), 6.0f);
    EXPECT_FLOAT_EQ(result.z(), 7.0f);
}

// --- Camera integration ---

TEST_F(FollowManipulatorTest, SetCameraDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(camera_));
}

TEST_F(FollowManipulatorTest, SetViewportSizeDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setViewportSize(1920.0f, 1080.0f));
}

TEST_F(FollowManipulatorTest, ResetStateIsNoOp) {
    manip_->setCamera(camera_);
    EXPECT_NO_FATAL_FAILURE(manip_->resetState());
}

TEST_F(FollowManipulatorTest, FitToAABBDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, -1.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 1.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

TEST_F(FollowManipulatorTest, GetWorldUnitsPerPixelIsNonNegative) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_GE(manip_->getWorldUnitsPerPixel(), 0.0f);
}

TEST_F(FollowManipulatorTest, UpdateWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    manip_->setTargetWorld({0.0f, 0.0f, 0.0f});
    EXPECT_NO_FATAL_FAILURE(manip_->update(0.016));
}

TEST_F(FollowManipulatorTest, HandleMouseScrollDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->onMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, 0.016));
}

TEST_F(FollowManipulatorTest, HandleTouchPinchDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::interaction::TouchPinch pinch{0.95f, 640.0f, 360.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->onTouchPinch(pinch, 0.016));
}

}  // namespace vne_interaction_test
