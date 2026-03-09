/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class ArcballManipulatorTest : public testing::Test {
   protected:
    void SetUp() override {
        manip_ = std::make_unique<vne::interaction::ArcballManipulator>();
        camera_ = vne::scene::CameraFactory::createPerspective(
            vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
        camera_->setViewport(1280.0f, 720.0f);
    }

    std::unique_ptr<vne::interaction::ArcballManipulator> manip_;
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
};

// --- Interface capability ---

TEST_F(ArcballManipulatorTest, SupportsPerspective) {
    EXPECT_TRUE(manip_->supportsPerspective());
}

TEST_F(ArcballManipulatorTest, SupportsOrthographic) {
    EXPECT_TRUE(manip_->supportsOrthographic());
}

// --- Default values ---

TEST_F(ArcballManipulatorTest, DefaultSceneScaleIsOne) {
    EXPECT_FLOAT_EQ(manip_->getSceneScale(), 1.0f);
}

TEST_F(ArcballManipulatorTest, DefaultOrbitDistanceIsFive) {
    EXPECT_FLOAT_EQ(manip_->getOrbitDistance(), 5.0f);
}

TEST_F(ArcballManipulatorTest, DefaultZoomMethodIsDollyToCoi) {
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eDollyToCoi);
}

TEST_F(ArcballManipulatorTest, DefaultRotationSpeedIsOne) {
    EXPECT_FLOAT_EQ(manip_->getRotationSpeed(), 1.0f);
}

TEST_F(ArcballManipulatorTest, DefaultPanSpeedIsOne) {
    EXPECT_FLOAT_EQ(manip_->getPanSpeed(), 1.0f);
}

TEST_F(ArcballManipulatorTest, DefaultZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.1f);
}

TEST_F(ArcballManipulatorTest, DefaultFovZoomSpeed) {
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.05f);
}

TEST_F(ArcballManipulatorTest, DefaultRotationDamping) {
    EXPECT_FLOAT_EQ(manip_->getRotationDamping(), 8.0f);
}

TEST_F(ArcballManipulatorTest, DefaultPanDamping) {
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 10.0f);
}

TEST_F(ArcballManipulatorTest, DefaultButtonMapRotateIsLeft) {
    const auto& map = manip_->getButtonMap();
    EXPECT_EQ(map.rotate, static_cast<int>(vne::interaction::MouseButton::eLeft));
}

TEST_F(ArcballManipulatorTest, DefaultButtonMapPanIsRight) {
    const auto& map = manip_->getButtonMap();
    EXPECT_EQ(map.pan, static_cast<int>(vne::interaction::MouseButton::eRight));
}

// --- Setters and getters ---

TEST_F(ArcballManipulatorTest, SetZoomMethodSceneScale) {
    manip_->setZoomMethod(vne::interaction::ZoomMethod::eSceneScale);
    EXPECT_EQ(manip_->getZoomMethod(), vne::interaction::ZoomMethod::eSceneScale);
}

TEST_F(ArcballManipulatorTest, SetRotationSpeedStoresValue) {
    manip_->setRotationSpeed(2.0f);
    EXPECT_FLOAT_EQ(manip_->getRotationSpeed(), 2.0f);
}

TEST_F(ArcballManipulatorTest, SetRotationSpeedClampsToZero) {
    manip_->setRotationSpeed(-0.5f);
    EXPECT_FLOAT_EQ(manip_->getRotationSpeed(), 0.0f);
}

TEST_F(ArcballManipulatorTest, SetPanSpeedStoresValue) {
    manip_->setPanSpeed(3.0f);
    EXPECT_FLOAT_EQ(manip_->getPanSpeed(), 3.0f);
}

TEST_F(ArcballManipulatorTest, SetPanSpeedClampsToZero) {
    manip_->setPanSpeed(-2.0f);
    EXPECT_FLOAT_EQ(manip_->getPanSpeed(), 0.0f);
}

TEST_F(ArcballManipulatorTest, SetZoomSpeedStoresValue) {
    manip_->setZoomSpeed(1.3f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 1.3f);
}

TEST_F(ArcballManipulatorTest, SetZoomSpeedClampsToMinimum) {
    manip_->setZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getZoomSpeed(), 0.01f);
}

TEST_F(ArcballManipulatorTest, SetFovZoomSpeedStoresValue) {
    manip_->setFovZoomSpeed(1.1f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 1.1f);
}

TEST_F(ArcballManipulatorTest, SetFovZoomSpeedClampsToMinimum) {
    manip_->setFovZoomSpeed(0.0f);
    EXPECT_FLOAT_EQ(manip_->getFovZoomSpeed(), 0.01f);
}

TEST_F(ArcballManipulatorTest, SetRotationDampingStoresValue) {
    manip_->setRotationDamping(6.0f);
    EXPECT_FLOAT_EQ(manip_->getRotationDamping(), 6.0f);
}

TEST_F(ArcballManipulatorTest, SetRotationDampingClampsToZero) {
    manip_->setRotationDamping(-1.0f);
    EXPECT_FLOAT_EQ(manip_->getRotationDamping(), 0.0f);
}

TEST_F(ArcballManipulatorTest, SetPanDampingStoresValue) {
    manip_->setPanDamping(15.0f);
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 15.0f);
}

TEST_F(ArcballManipulatorTest, SetPanDampingClampsToZero) {
    manip_->setPanDamping(-4.0f);
    EXPECT_FLOAT_EQ(manip_->getPanDamping(), 0.0f);
}

TEST_F(ArcballManipulatorTest, SetButtonMapStoresValues) {
    vne::interaction::ButtonMap button_map;
    button_map.rotate = static_cast<int>(vne::interaction::MouseButton::eMiddle);
    button_map.pan = static_cast<int>(vne::interaction::MouseButton::eLeft);
    manip_->setButtonMap(button_map);
    const auto& stored = manip_->getButtonMap();
    EXPECT_EQ(stored.rotate, static_cast<int>(vne::interaction::MouseButton::eMiddle));
    EXPECT_EQ(stored.pan, static_cast<int>(vne::interaction::MouseButton::eLeft));
}

// --- Camera integration ---

TEST_F(ArcballManipulatorTest, SetCameraDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setCamera(camera_));
}

TEST_F(ArcballManipulatorTest, SetViewportSizeDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(manip_->setViewportSize(1920.0f, 1080.0f));
}

TEST_F(ArcballManipulatorTest, ResetStateWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->resetState());
}

TEST_F(ArcballManipulatorTest, FitToAABBDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, -1.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 1.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

TEST_F(ArcballManipulatorTest, FitToAABBSetsPositiveSceneScale) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-3.0f, -3.0f, -3.0f};
    vne::math::Vec3f max_world{3.0f, 3.0f, 3.0f};
    manip_->fitToAABB(min_world, max_world);
    EXPECT_GT(manip_->getSceneScale(), 0.0f);
}

TEST_F(ArcballManipulatorTest, GetWorldUnitsPerPixelIsNonNegative) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_GE(manip_->getWorldUnitsPerPixel(), 0.0f);
}

TEST_F(ArcballManipulatorTest, UpdateWithCameraDoesNotCrash) {
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    EXPECT_NO_FATAL_FAILURE(manip_->update(0.016));
}

TEST_F(ArcballManipulatorTest, WithOrthographicCameraFitToAABBDoesNotCrash) {
    auto ortho_cam = vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
    manip_->setCamera(ortho_cam);
    manip_->setViewportSize(1280.0f, 720.0f);
    vne::math::Vec3f min_world{-1.0f, -1.0f, -1.0f};
    vne::math::Vec3f max_world{1.0f, 1.0f, 1.0f};
    EXPECT_NO_FATAL_FAILURE(manip_->fitToAABB(min_world, max_world));
}

}  // namespace vne_interaction_test
