/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class OrbitManipulatorTest : public testing::Test {
   protected:
    void SetUp() override {
        manip_ = std::make_unique<vne::interaction::OrbitManipulator>();
        camera_ = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
        camera_->setViewport(1280.0f, 720.0f);
    }

    std::unique_ptr<vne::interaction::OrbitManipulator> manip_;
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
};

// --- Interface capability ---

TEST_F(OrbitManipulatorTest, SupportsPerspective) {
    EXPECT_TRUE(manip_->supportsPerspective());
}

TEST_F(OrbitManipulatorTest, SupportsOrthographic) {
    EXPECT_TRUE(manip_->supportsOrthographic());
}

// --- Default values ---

TEST_F(OrbitManipulatorTest, DefaultSceneScaleIsOne) {
    EXPECT_FLOAT_EQ(manip_->getSceneScale(), 1.0f);
}

TEST_F(OrbitManipulatorTest, DefaultOrbitDistanceIsFive) {
    EXPECT_FLOAT_EQ(manip_->getOrbitDistance(), 5.0f);
}

TEST_F(OrbitManipulatorTest, DefaultZoomMethodIsDollyToCoi) {
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eDollyToCoi);
}

TEST_F(OrbitManipulatorTest, DefaultRotationSpeed) {
    EXPECT_FLOAT_EQ(manip_->getRotationSpeed(), 0.2f);
}

TEST_F(OrbitManipulatorTest, DefaultPanSpeed) {
    EXPECT_FLOAT_EQ(manip_->getPanSpeed(), 1.0f);
}

TEST_F(OrbitManipulatorTest, DefaultZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.1f);
}

TEST_F(OrbitManipulatorTest, DefaultFovZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.05f);
}

TEST_F(OrbitManipulatorTest, DefaultRotationDamping) {
    EXPECT_FLOAT_EQ(manip_->getRotationDamping(), 8.0f);
}

TEST_F(OrbitManipulatorTest, DefaultPanDamping) {
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 10.0f);
}

TEST_F(OrbitManipulatorTest, DefaultButtonMapRotateIsLeft) {
    const auto& map = manip_->getButtonMap();
    EXPECT_EQ(map.rotate, static_cast<int>(vne::interaction::MouseButton::eLeft));
}

TEST_F(OrbitManipulatorTest, DefaultButtonMapPanIsRight) {
    const auto& map = manip_->getButtonMap();
    EXPECT_EQ(map.pan, static_cast<int>(vne::interaction::MouseButton::eRight));
}

// --- Setters and getters ---

TEST_F(OrbitManipulatorTest, SetZoomMethodSceneScale) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eSceneScale);
}

TEST_F(OrbitManipulatorTest, SetZoomMethodChangeFov) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eChangeFov);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eChangeFov);
}

TEST_F(OrbitManipulatorTest, SetRotationSpeedStoresValue) {
    manip_->setRotationSpeed(0.5f);
    EXPECT_FLOAT_EQ(manip_->getRotationSpeed(), 0.5f);
}

TEST_F(OrbitManipulatorTest, SetRotationSpeedClampsToZero) {
    manip_->setRotationSpeed(-1.0f);
    EXPECT_FLOAT_EQ(manip_->getRotationSpeed(), 0.0f);
}

TEST_F(OrbitManipulatorTest, SetPanSpeedStoresValue) {
    manip_->setPanSpeed(2.0f);
    EXPECT_FLOAT_EQ(manip_->getPanSpeed(), 2.0f);
}

TEST_F(OrbitManipulatorTest, SetPanSpeedClampsToZero) {
    manip_->setPanSpeed(-0.5f);
    EXPECT_FLOAT_EQ(manip_->getPanSpeed(), 0.0f);
}

TEST_F(OrbitManipulatorTest, SetZoomSpeedStoresValue) {
    manip_->setZoomSpeed(1.5f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.5f);
}

TEST_F(OrbitManipulatorTest, SetZoomSpeedClampsToMinimum) {
    manip_->setZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.01f);
}

TEST_F(OrbitManipulatorTest, SetFovZoomSpeedStoresValue) {
    manip_->setFovZoomSpeed(1.2f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.2f);
}

TEST_F(OrbitManipulatorTest, SetFovZoomSpeedClampsToMinimum) {
    manip_->setFovZoomSpeed(-1.0f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 0.01f);
}

TEST_F(OrbitManipulatorTest, SetRotationDampingStoresValue) {
    manip_->setRotationDamping(5.0f);
    EXPECT_FLOAT_EQ(manip_->getRotationDamping(), 5.0f);
}

TEST_F(OrbitManipulatorTest, SetRotationDampingClampsToZero) {
    manip_->setRotationDamping(-3.0f);
    EXPECT_FLOAT_EQ(manip_->getRotationDamping(), 0.0f);
}

TEST_F(OrbitManipulatorTest, SetPanDampingStoresValue) {
    manip_->setPanDamping(12.0f);
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 12.0f);
}

TEST_F(OrbitManipulatorTest, SetPanDampingClampsToZero) {
    manip_->setPanDamping(-5.0f);
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 0.0f);
}

TEST_F(OrbitManipulatorTest, SetButtonMapStoresValues) {
    vne::interaction::OrbitManipulator::ButtonMap map;
    map.rotate = static_cast<int>(vne::interaction::MouseButton::eMiddle);
    map.pan = static_cast<int>(vne::interaction::MouseButton::eLeft);
    manip_->setButtonMap(map);
    const auto& stored = manip_->getButtonMap();
    EXPECT_EQ(stored.rotate, static_cast<int>(vne::interaction::MouseButton::eMiddle));
    EXPECT_EQ(stored.pan, static_cast<int>(vne::interaction::MouseButton::eLeft));
}

// --- Camera integration ---

TEST_F(OrbitManipulatorTest, SetCameraDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(camera_));
}

TEST_F(OrbitManipulatorTest, SetViewportSizeDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setViewportSize(1920.0f, 1080.0f));
}

TEST_F(OrbitManipulatorTest, ResetStateDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->resetState());
}

TEST_F(OrbitManipulatorTest, FitToAABBDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, -1.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 1.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

TEST_F(OrbitManipulatorTest, FitToAABBSetsPositiveSceneScale) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-2.0f, -2.0f, -2.0f};
    vne::math::Vec3f max_world{2.0f, 2.0f, 2.0f};
    manip_->fitToAABB(min_world, max_world);
    EXPECT_GT(manip_->getSceneScale(), 0.0f);
}

TEST_F(OrbitManipulatorTest, GetWorldUnitsPerPixelIsNonNegative) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_GE(manip_->getWorldUnitsPerPixel(), 0.0f);
}

TEST_F(OrbitManipulatorTest, UpdateWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->update(0.016));
}

TEST_F(OrbitManipulatorTest, HandleMouseMoveWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->handleMouseMove(640.0f, 360.0f, 1.0f, -1.0f, 0.016));
}

TEST_F(OrbitManipulatorTest, HandleMouseScrollWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, 0.016));
}

TEST_F(OrbitManipulatorTest, SetViewDirectionFrontDoesNotCrash) {
    manip_->setCamera(camera_);
    EXPECT_NO_FATAL_FAILURE(manip_->setViewDirection(vne::interaction::ViewDirection::eFront));
}

TEST_F(OrbitManipulatorTest, SetViewDirectionTopDoesNotCrash) {
    manip_->setCamera(camera_);
    EXPECT_NO_FATAL_FAILURE(manip_->setViewDirection(vne::interaction::ViewDirection::eTop));
}

TEST_F(OrbitManipulatorTest, SetOrbitDistanceStoresPositiveValue) {
    manip_->setCamera(camera_);
    manip_->setOrbitDistance(10.0f);
    EXPECT_FLOAT_EQ(manip_->getOrbitDistance(), 10.0f);
}

TEST_F(OrbitManipulatorTest, WithOrthographicCameraFitToAABBDoesNotCrash) {
    auto ortho_cam = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    manip_->setCamera(ortho_cam);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, -1.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 1.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

}  // namespace vne_interaction_test
