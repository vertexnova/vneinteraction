/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class FpsManipulatorTest : public testing::Test {
   protected:
    void SetUp() override {
        manip_ = std::make_unique<vne::interaction::FpsManipulator>();
        camera_ = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
        camera_->setViewport(1280.0f, 720.0f);
    }

    std::unique_ptr<vne::interaction::FpsManipulator> manip_;
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
};

// --- Interface capability ---

TEST_F(FpsManipulatorTest, SupportsPerspective) {
    EXPECT_TRUE(manip_->supportsPerspective());
}

TEST_F(FpsManipulatorTest, SupportsOrthographic) {
    EXPECT_TRUE(manip_->supportsOrthographic());
}

// --- Default values ---

TEST_F(FpsManipulatorTest, DefaultSceneScaleIsOne) {
    EXPECT_FLOAT_EQ(manip_->getSceneScale(), 1.0f);
}

TEST_F(FpsManipulatorTest, DefaultMoveSpeed) {
    EXPECT_FLOAT_EQ(manip_->getMoveSpeed(), 3.0f);
}

TEST_F(FpsManipulatorTest, DefaultMouseSensitivity) {
    EXPECT_FLOAT_EQ(manip_->getMouseSensitivity(), 0.15f);
}

TEST_F(FpsManipulatorTest, DefaultZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.5f);
}

TEST_F(FpsManipulatorTest, DefaultFovZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.05f);
}

TEST_F(FpsManipulatorTest, DefaultSprintMultiplier) {
    EXPECT_FLOAT_EQ(manip_->getSprintMultiplier(), 4.0f);
}

TEST_F(FpsManipulatorTest, DefaultSlowMultiplier) {
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 0.2f);
}

TEST_F(FpsManipulatorTest, DefaultZoomMethodIsDollyToCoi) {
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eDollyToCoi);
}

// --- Setters with clamping ---

TEST_F(FpsManipulatorTest, SetMoveSpeedStoresValue) {
    manip_->setMoveSpeed(5.0f);
    EXPECT_FLOAT_EQ(manip_->getMoveSpeed(), 5.0f);
}

TEST_F(FpsManipulatorTest, SetMoveSpeedClampsToZero) {
    manip_->setMoveSpeed(-1.0f);
    EXPECT_FLOAT_EQ(manip_->getMoveSpeed(), 0.0f);
}

TEST_F(FpsManipulatorTest, SetMouseSensitivityStoresValue) {
    manip_->setMouseSensitivity(0.3f);
    EXPECT_FLOAT_EQ(manip_->getMouseSensitivity(), 0.3f);
}

TEST_F(FpsManipulatorTest, SetMouseSensitivityClampsToZero) {
    manip_->setMouseSensitivity(-0.1f);
    EXPECT_FLOAT_EQ(manip_->getMouseSensitivity(), 0.0f);
}

TEST_F(FpsManipulatorTest, SetZoomSpeedStoresValue) {
    manip_->setZoomSpeed(1.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.0f);
}

TEST_F(FpsManipulatorTest, SetZoomSpeedClampsToMinimum) {
    manip_->setZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.01f);
}

TEST_F(FpsManipulatorTest, SetFovZoomSpeedStoresValue) {
    manip_->setFovZoomSpeed(1.2f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.2f);
}

TEST_F(FpsManipulatorTest, SetFovZoomSpeedClampsToMinimum) {
    manip_->setFovZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 0.01f);
}

TEST_F(FpsManipulatorTest, SetSprintMultiplierStoresValue) {
    manip_->setSprintMultiplier(8.0f);
    EXPECT_FLOAT_EQ(manip_->getSprintMultiplier(), 8.0f);
}

TEST_F(FpsManipulatorTest, SetSprintMultiplierClampsToOne) {
    manip_->setSprintMultiplier(0.5f);
    EXPECT_FLOAT_EQ(manip_->getSprintMultiplier(), 1.0f);
}

TEST_F(FpsManipulatorTest, SetSlowMultiplierStoresValue) {
    manip_->setSlowMultiplier(0.1f);
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 0.1f);
}

TEST_F(FpsManipulatorTest, SetSlowMultiplierClampsToZero) {
    manip_->setSlowMultiplier(-0.5f);
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 0.0f);
}

TEST_F(FpsManipulatorTest, SetSlowMultiplierClampsToOne) {
    manip_->setSlowMultiplier(2.0f);
    EXPECT_FLOAT_EQ(manip_->getSlowMultiplier(), 1.0f);
}

TEST_F(FpsManipulatorTest, SetZoomMethodChangeFov) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eChangeFov);
}

// --- Camera integration ---

TEST_F(FpsManipulatorTest, SetCameraDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(camera_));
}

TEST_F(FpsManipulatorTest, SetViewportSizeDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setViewportSize(1920.0f, 1080.0f));
}

TEST_F(FpsManipulatorTest, ResetStateDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->resetState());
}

TEST_F(FpsManipulatorTest, FitToAABBDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, -1.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 1.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

TEST_F(FpsManipulatorTest, FitToAABBSetsPositiveSceneScale) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-5.0f, -5.0f, -5.0f};
    vne::math::Vec3f max_world{5.0f, 5.0f, 5.0f};
    manip_->fitToAABB(min_world, max_world);
    EXPECT_GT(manip_->getSceneScale(), 0.0f);
}

TEST_F(FpsManipulatorTest, GetWorldUnitsPerPixelIsNonNegative) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_GE(manip_->getWorldUnitsPerPixel(), 0.0f);
}

TEST_F(FpsManipulatorTest, UpdateWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->update(0.016));
}

TEST_F(FpsManipulatorTest, HandleMouseButtonPressDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->onMouseButton(static_cast<int>(vne::interaction::MouseButton::eRight),
                                                      true,
                                                      640.0f,
                                                      360.0f,
                                                      0.016));
}

TEST_F(FpsManipulatorTest, HandleKeyboardWDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->onKeyboard(87, true, 0.016));
}

TEST_F(FpsManipulatorTest, HandleTouchPanDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::interaction::TouchPan pan{10.0f, -5.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->onTouchPan(pan, 0.016));
}

TEST_F(FpsManipulatorTest, HandleMouseScrollDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->onMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, 0.016));
}

}  // namespace vne_interaction_test
