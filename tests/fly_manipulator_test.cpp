/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class FlyManipulatorTest : public testing::Test {
   protected:
    void SetUp() override {
        manip_ = std::make_unique<vne::interaction::FlyManipulator>();
        camera_ = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
        camera_->setViewport(1280.0f, 720.0f);
    }

    std::unique_ptr<vne::interaction::FlyManipulator> manip_;
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
};

// --- Interface capability ---

TEST_F(FlyManipulatorTest, SupportsPerspective) {
    EXPECT_TRUE(manip_->supportsPerspective());
}

TEST_F(FlyManipulatorTest, SupportsOrthographic) {
    EXPECT_TRUE(manip_->supportsOrthographic());
}

// --- Default values ---

TEST_F(FlyManipulatorTest, DefaultSceneScaleIsOne) {
    EXPECT_FLOAT_EQ(manip_->getSceneScale(), 1.0f);
}

TEST_F(FlyManipulatorTest, DefaultMoveSpeed) {
    EXPECT_FLOAT_EQ(manip_->getMoveSpeed(), 3.0f);
}

TEST_F(FlyManipulatorTest, DefaultMouseSensitivity) {
    EXPECT_FLOAT_EQ(manip_->getMouseSensitivity(), 0.15f);
}

TEST_F(FlyManipulatorTest, DefaultZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.5f);
}

TEST_F(FlyManipulatorTest, DefaultFovZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.05f);
}

TEST_F(FlyManipulatorTest, DefaultSprintMultiplier) {
    EXPECT_FLOAT_EQ(manip_->getSprintMultiplier(), 4.0f);
}

TEST_F(FlyManipulatorTest, DefaultSlowMultiplier) {
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 0.2f);
}

TEST_F(FlyManipulatorTest, DefaultZoomMethodIsDollyToCoi) {
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eDollyToCoi);
}

// --- Setters with clamping ---

TEST_F(FlyManipulatorTest, SetMoveSpeedStoresValue) {
    manip_->setMoveSpeed(10.0f);
    EXPECT_FLOAT_EQ(manip_->getMoveSpeed(), 10.0f);
}

TEST_F(FlyManipulatorTest, SetMoveSpeedClampsToZero) {
    manip_->setMoveSpeed(-2.0f);
    EXPECT_FLOAT_EQ(manip_->getMoveSpeed(), 0.0f);
}

TEST_F(FlyManipulatorTest, SetMouseSensitivityStoresValue) {
    manip_->setMouseSensitivity(0.25f);
    EXPECT_FLOAT_EQ(manip_->getMouseSensitivity(), 0.25f);
}

TEST_F(FlyManipulatorTest, SetMouseSensitivityClampsToZero) {
    manip_->setMouseSensitivity(-0.05f);
    EXPECT_FLOAT_EQ(manip_->getMouseSensitivity(), 0.0f);
}

TEST_F(FlyManipulatorTest, SetZoomSpeedStoresValue) {
    manip_->setZoomSpeed(0.8f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.8f);
}

TEST_F(FlyManipulatorTest, SetZoomSpeedClampsToMinimum) {
    manip_->setZoomSpeed(-1.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.01f);
}

TEST_F(FlyManipulatorTest, SetFovZoomSpeedStoresValue) {
    manip_->setFovZoomSpeed(1.15f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.15f);
}

TEST_F(FlyManipulatorTest, SetFovZoomSpeedClampsToMinimum) {
    manip_->setFovZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 0.01f);
}

TEST_F(FlyManipulatorTest, SetSprintMultiplierStoresValue) {
    manip_->setSprintMultiplier(6.0f);
    EXPECT_FLOAT_EQ(manip_->getSprintMultiplier(), 6.0f);
}

TEST_F(FlyManipulatorTest, SetSprintMultiplierClampsToOne) {
    manip_->setSprintMultiplier(0.0f);
    EXPECT_FLOAT_EQ(manip_->getSprintMultiplier(), 1.0f);
}

TEST_F(FlyManipulatorTest, SetSlowMultiplierStoresValue) {
    manip_->setSlowMultiplier(0.5f);
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 0.5f);
}

TEST_F(FlyManipulatorTest, SetSlowMultiplierClampsToZero) {
    manip_->setSlowMultiplier(-1.0f);
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 0.0f);
}

TEST_F(FlyManipulatorTest, SetSlowMultiplierClampsToOne) {
    manip_->setSlowMultiplier(1.5f);
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 1.0f);
}

TEST_F(FlyManipulatorTest, SetZoomMethodSceneScale) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eSceneScale);
}

// --- Camera integration ---

TEST_F(FlyManipulatorTest, SetCameraDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(camera_));
}

TEST_F(FlyManipulatorTest, SetViewportSizeDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setViewportSize(2560.0f, 1440.0f));
}

TEST_F(FlyManipulatorTest, ResetStateDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->resetState());
}

TEST_F(FlyManipulatorTest, FitToAABBDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-2.0f, -2.0f, -2.0f};
    vne::math::Vec3f max_world{2.0f, 2.0f, 2.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

TEST_F(FlyManipulatorTest, FitToAABBSetsPositiveSceneScale) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-4.0f, -4.0f, -4.0f};
    vne::math::Vec3f max_world{4.0f, 4.0f, 4.0f};
    manip_->fitToAABB(min_world, max_world);
    EXPECT_GT(manip_->getSceneScale(), 0.0f);
}

TEST_F(FlyManipulatorTest, GetWorldUnitsPerPixelIsNonNegative) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_GE(manip_->getWorldUnitsPerPixel(), 0.0f);
}

TEST_F(FlyManipulatorTest, UpdateWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->update(0.016));
}

TEST_F(FlyManipulatorTest, HandleMouseButtonPressDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eRight),
                                                      true,
                                                      640.0f,
                                                      360.0f,
                                                      0.016));
}

TEST_F(FlyManipulatorTest, HandleKeyboardWDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->handleKeyboard(87, true, 0.016));
}

TEST_F(FlyManipulatorTest, HandleTouchPinchDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::interaction::TouchPinch pinch{1.05f, 640.0f, 360.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->handleTouchPinch(pinch, 0.016));
}

}  // namespace vne_interaction_test
