/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class OrthoPanZoomManipulatorTest : public testing::Test {
   protected:
    void SetUp() override {
        manip_ = std::make_unique<vne::interaction::OrthoPanZoomManipulator>();
        ortho_cam_ = vne::scene::CameraFactory::createOrthographic(
            vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    }

    std::unique_ptr<vne::interaction::OrthoPanZoomManipulator> manip_;
    std::shared_ptr<vne::scene::OrthographicCamera> ortho_cam_;
};

// --- Interface capability ---

TEST_F(OrthoPanZoomManipulatorTest, DoesNotSupportPerspective) {
    EXPECT_FALSE(manip_->supportsPerspective());
}

TEST_F(OrthoPanZoomManipulatorTest, SupportsOrthographic) {
    EXPECT_TRUE(manip_->supportsOrthographic());
}

// --- Default values ---

TEST_F(OrthoPanZoomManipulatorTest, DefaultSceneScaleIsOne) {
    EXPECT_FLOAT_EQ(manip_->getSceneScale(), 1.0f);
}

TEST_F(OrthoPanZoomManipulatorTest, DefaultZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.1f);
}

TEST_F(OrthoPanZoomManipulatorTest, DefaultPanDamping) {
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 10.0f);
}

TEST_F(OrthoPanZoomManipulatorTest, DefaultZoomMethodIsDollyToCoi) {
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eDollyToCoi);
}

// --- Setters and getters ---

TEST_F(OrthoPanZoomManipulatorTest, SetZoomSpeedStoresValue) {
    manip_->setZoomSpeed(1.2f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.2f);
}

TEST_F(OrthoPanZoomManipulatorTest, SetZoomSpeedClampsToMinimum) {
    manip_->setZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.01f);
}

TEST_F(OrthoPanZoomManipulatorTest, SetPanDampingStoresValue) {
    manip_->setPanDamping(5.0f);
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 5.0f);
}

TEST_F(OrthoPanZoomManipulatorTest, SetPanDampingClampsToZero) {
    manip_->setPanDamping(-4.0f);
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 0.0f);
}

TEST_F(OrthoPanZoomManipulatorTest, SetZoomMethodSceneScale) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eSceneScale);
}

// --- Camera integration ---

TEST_F(OrthoPanZoomManipulatorTest, SetCameraWithOrthographicDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(ortho_cam_));
}

TEST_F(OrthoPanZoomManipulatorTest, SetViewportSizeDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setViewportSize(1280.0f, 720.0f));
}

TEST_F(OrthoPanZoomManipulatorTest, ResetStateWithOrthoCameraDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->resetState());
}

TEST_F(OrthoPanZoomManipulatorTest, FitToAABBWithOrthoCameraDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, 0.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 0.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

TEST_F(OrthoPanZoomManipulatorTest, FitToAABBSetsPositiveSceneScale) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-5.0f, -5.0f, 0.0f};
    vne::math::Vec3f max_world{5.0f, 5.0f, 0.0f};
    manip_->fitToAABB(min_world, max_world);
    EXPECT_GT(manip_->getSceneScale(), 0.0f);
}

TEST_F(OrthoPanZoomManipulatorTest, GetWorldUnitsPerPixelIsNonNegative) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_GE(manip_->getWorldUnitsPerPixel(), 0.0f);
}

TEST_F(OrthoPanZoomManipulatorTest, UpdateWithOrthoCameraDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->update(0.016));
}

TEST_F(OrthoPanZoomManipulatorTest, HandleMouseScrollZoomsDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, 0.016));
}

TEST_F(OrthoPanZoomManipulatorTest, HandleMouseButtonMiddlePanDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->handleMouseButton(static_cast<int>(vne::interaction::MouseButton::eMiddle),
                                                      true,
                                                      640.0f,
                                                      360.0f,
                                                      0.016));
}

TEST_F(OrthoPanZoomManipulatorTest, HandleTouchPinchDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::interaction::TouchPinch pinch{0.9f, 640.0f, 360.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->handleTouchPinch(pinch, 0.016));
}

TEST_F(OrthoPanZoomManipulatorTest, HandleTouchPanDoesNotCrash) {
    manip_->setCamera(ortho_cam_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::interaction::TouchPan pan{5.0f, 3.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->handleTouchPan(pan, 0.016));
}

TEST_F(OrthoPanZoomManipulatorTest, SetCameraWithPerspectiveDoesNotCrash) {
    auto persp_cam = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(persp_cam));
}

}  // namespace vne_interaction_test
