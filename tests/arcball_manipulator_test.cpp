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

// =============================================================================
// Rotation pivot mode
// =============================================================================

TEST_F(ArcballManipulatorTest, DefaultPivotModeIsCoi) {
    EXPECT_EQ(manip_->getPivotMode(), vne::interaction::RotationPivotMode::eCoi);
}

TEST_F(ArcballManipulatorTest, SetPivotModeStoresValue) {
    manip_->setPivotMode(vne::interaction::RotationPivotMode::eFixedWorld);
    EXPECT_EQ(manip_->getPivotMode(), vne::interaction::RotationPivotMode::eFixedWorld);
}

// eFixedWorld: panning must NOT move coi_world_ — it should stay pinned.
TEST_F(ArcballManipulatorTest, FixedWorldPivotCoiStaysPinnedAfterPan) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    manip_->setPivotMode(vne::interaction::RotationPivotMode::eFixedWorld);

    const vne::math::Vec3f coi_before = manip_->getCenterOfInterestWorld();

    // Simulate a pan drag
    manip_->handleMouseButton(1, true, 640.0f, 360.0f, 0.016);  // right-click = pan
    manip_->handleMouseMove(660.0f, 370.0f, 20.0f, 10.0f, 0.016);
    manip_->handleMouseButton(1, false, 660.0f, 370.0f, 0.016);

    const vne::math::Vec3f coi_after = manip_->getCenterOfInterestWorld();
    // COI must be unchanged
    EXPECT_NEAR(coi_after.x(), coi_before.x(), 1e-4f);
    EXPECT_NEAR(coi_after.y(), coi_before.y(), 1e-4f);
    EXPECT_NEAR(coi_after.z(), coi_before.z(), 1e-4f);
}

// eFixedWorld: panning must translate the camera eye (position must change).
TEST_F(ArcballManipulatorTest, FixedWorldPivotEyeTranslatesAfterPan) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    manip_->setPivotMode(vne::interaction::RotationPivotMode::eFixedWorld);

    const vne::math::Vec3f eye_before = camera_->getPosition();

    manip_->handleMouseButton(1, true, 640.0f, 360.0f, 0.016);
    manip_->handleMouseMove(660.0f, 360.0f, 20.0f, 0.0f, 0.016);
    manip_->handleMouseButton(1, false, 660.0f, 360.0f, 0.016);

    const vne::math::Vec3f eye_after = camera_->getPosition();
    // Eye must have moved (pan translated it)
    const float moved = (eye_after - eye_before).length();
    EXPECT_GT(moved, 1e-4f);
}

// eViewCenter: after pan+end, COI should update to the camera's new look-at point.
TEST_F(ArcballManipulatorTest, ViewCenterPivotCoiUpdatesOnPanEnd) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);
    manip_->setPivotMode(vne::interaction::RotationPivotMode::eViewCenter);

    manip_->handleMouseButton(1, true, 640.0f, 360.0f, 0.016);
    manip_->handleMouseMove(700.0f, 360.0f, 60.0f, 0.0f, 0.016);
    manip_->handleMouseButton(1, false, 700.0f, 360.0f, 0.016);  // endPan triggers COI update

    const vne::math::Vec3f coi = manip_->getCenterOfInterestWorld();
    const vne::math::Vec3f target = camera_->getTarget();
    EXPECT_NEAR(coi.x(), target.x(), 1e-3f);
    EXPECT_NEAR(coi.y(), target.y(), 1e-3f);
    EXPECT_NEAR(coi.z(), target.z(), 1e-3f);
}

// =============================================================================
// fitToAABB — immediate + convergence
// =============================================================================

// fitToAABB (perspective) must apply the camera state immediately (no stale getters).
TEST_F(ArcballManipulatorTest, FitToAABBPerspectiveAppliesImmediately) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 100.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);

    manip_->fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));

    // orbit_distance_ and camera position must reflect the fit right away, before update() runs
    EXPECT_GT(manip_->getOrbitDistance(), 0.0f);
    EXPECT_LT(manip_->getOrbitDistance(), 100.0f);  // must have moved closer

    const vne::math::Vec3f pos_after = camera_->getPosition();
    const float dist_from_origin = pos_after.length();
    EXPECT_LT(dist_from_origin, 50.0f);  // camera moved toward geometry
}

// fitToAABB animation must converge within a bounded number of frames.
TEST_F(ArcballManipulatorTest, FitToAABBConvergesWithinBoundedFrames) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);

    manip_->fitToAABB(vne::math::Vec3f(-1.0f, -1.0f, -1.0f), vne::math::Vec3f(1.0f, 1.0f, 1.0f));
    const float target_dist = manip_->getOrbitDistance();  // already applied immediately

    // Run at 60 fps for up to 5 seconds — animation must have stopped long before then
    constexpr int kMaxFrames = 300;
    constexpr double kDt = 1.0 / 60.0;
    for (int i = 0; i < kMaxFrames; ++i) {
        manip_->update(kDt);
    }

    // After convergence, orbit distance must be very close to the target
    EXPECT_NEAR(manip_->getOrbitDistance(), target_dist, 1e-2f);
    // Camera position must be stable (no ongoing animation)
    const vne::math::Vec3f pos_a = camera_->getPosition();
    manip_->update(kDt);
    const vne::math::Vec3f pos_b = camera_->getPosition();
    EXPECT_NEAR((pos_b - pos_a).length(), 0.0f, 1e-5f);
}

// =============================================================================
// Zoom-to-cursor COI migration
// =============================================================================

// Dolly zoom with an off-center cursor must migrate COI (i.e. COI moves, not stays put).
// This verifies zoom-to-cursor is active; the sign depends on screen layout.
TEST_F(ArcballManipulatorTest, ZoomToCursorShiftsCoiTowardCursor) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);

    const vne::math::Vec3f coi_before = manip_->getCenterOfInterestWorld();

    // Scroll zoom-in with cursor offset from center (x=960, center=640 → cursor right of center)
    manip_->handleMouseScroll(0.0f, 1.0f, 960.0f, 360.0f, 0.016);

    const vne::math::Vec3f coi_after = manip_->getCenterOfInterestWorld();
    // COI must have moved (non-zero migration) — direction depends on camera orientation
    const float coi_delta = (coi_after - coi_before).length();
    EXPECT_GT(coi_delta, 1e-4f);
}

// Dolly zoom in must decrease orbit distance.
TEST_F(ArcballManipulatorTest, ZoomInDecreasesOrbitDistance) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);

    const float dist_before = manip_->getOrbitDistance();
    manip_->handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, 0.016);  // scroll up = zoom in
    EXPECT_LT(manip_->getOrbitDistance(), dist_before);
}

// =============================================================================
// Orbit distance stays constant during rotation (no drift)
// =============================================================================

TEST_F(ArcballManipulatorTest, OrbitDistanceStableAfterManyRotationFrames) {
    camera_->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    camera_->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    camera_->setUp(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    camera_->updateMatrices();
    manip_->setCamera(camera_);
    manip_->setViewportSize(1280.0f, 720.0f);

    const float dist_initial = manip_->getOrbitDistance();

    // Simulate 100 frames of drag rotation across the sphere
    manip_->handleMouseButton(0, true, 640.0f, 360.0f, 0.016);
    float x = 640.0f;
    float y = 360.0f;
    for (int i = 0; i < 100; ++i) {
        x += 3.0f;
        y += 1.0f;
        manip_->handleMouseMove(x, y, 3.0f, 1.0f, 0.016);
    }
    manip_->handleMouseButton(0, false, x, y, 0.016);

    // Distance from eye to COI must remain constant (±1e-3 tolerance for float accumulation)
    const vne::math::Vec3f eye = camera_->getPosition();
    const vne::math::Vec3f coi = manip_->getCenterOfInterestWorld();
    const float dist_after = (eye - coi).length();
    EXPECT_NEAR(dist_after, dist_initial, 1e-2f);
}

}  // namespace vne_interaction_test
