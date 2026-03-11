/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Behavioral correctness and API robustness tests.
 * Verifies actual camera movement, not just "no crash".
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/camera_behavior.h"
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/interaction/follow_manipulator.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"
#include "vertexnova/interaction/detail/camera_manipulator_base.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <memory>
#include <cmath>

namespace vne_interaction_test {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    auto cam = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    cam->setViewport(1280.0f, 720.0f);
    return cam;
}

static std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
}

// ---------------------------------------------------------------------------
// isEnabled / setEnabled on ICameraManipulator
// ---------------------------------------------------------------------------

TEST(ApiRobustness, ManipulatorIsEnabledDefaultTrue) {
    vne::interaction::OrbitManipulator manip;
    EXPECT_TRUE(manip.isEnabled());
}

TEST(ApiRobustness, ManipulatorSetEnabledFalseReflected) {
    vne::interaction::OrbitManipulator manip;
    manip.setEnabled(false);
    EXPECT_FALSE(manip.isEnabled());
}

TEST(ApiRobustness, DisabledManipulatorDoesNotMoveCamera) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);
    manip.setEnabled(false);

    const vne::math::Vec3f pos_before = cam->getPosition();
    // Simulate a left-button drag that would normally rotate
    manip.handleMouseButton(0, true, 640.0f, 360.0f, 0.016);
    manip.handleMouseMove(680.0f, 360.0f, 40.0f, 0.0f, 0.016);
    manip.handleMouseButton(0, false, 680.0f, 360.0f, 0.016);
    manip.update(0.016);

    const vne::math::Vec3f pos_after = cam->getPosition();
    EXPECT_NEAR((pos_after - pos_before).length(), 0.0f, 1e-4f) << "Disabled manipulator must not move camera";
}

// ---------------------------------------------------------------------------
// setViewportSize minimum clamp
// ---------------------------------------------------------------------------

TEST(ApiRobustness, SetViewportSizeZeroClampedToOne) {
    vne::interaction::OrbitManipulator manip;
    auto cam = makePerspCamera();
    manip.setCamera(cam);
    // Zero viewport must not cause division by zero in getWorldUnitsPerPixel
    manip.setViewportSize(0.0f, 0.0f);
    EXPECT_GT(manip.getWorldUnitsPerPixel(), 0.0f);
    EXPECT_TRUE(std::isfinite(manip.getWorldUnitsPerPixel()));
}

TEST(ApiRobustness, SetViewportSizeNegativeClampedToOne) {
    vne::interaction::FpsManipulator manip;
    auto cam = makePerspCamera();
    manip.setCamera(cam);
    manip.setViewportSize(-100.0f, -50.0f);
    EXPECT_TRUE(std::isfinite(manip.getWorldUnitsPerPixel()));
}

// ---------------------------------------------------------------------------
// Factory never returns nullptr for valid enum values
// ---------------------------------------------------------------------------

TEST(ApiRobustness, FactoryNeverReturnsNullForValidTypes) {
    vne::interaction::CameraManipulatorFactory factory;
    using T = vne::interaction::CameraManipulatorType;
    for (auto type : {T::eOrbit, T::eArcball, T::eFps, T::eFly, T::eOrthoPanZoom, T::eFollow}) {
        EXPECT_NE(factory.create(type), nullptr) << "create() returned nullptr for type " << static_cast<int>(type);
    }
}

// ---------------------------------------------------------------------------
// getZoomSpeed visible on ICameraManipulator interface
// ---------------------------------------------------------------------------

TEST(ApiRobustness, GetZoomSpeedAccessibleViaInterface) {
    vne::interaction::CameraManipulatorFactory factory;
    std::shared_ptr<vne::interaction::ICameraManipulator> manip =
        factory.create(vne::interaction::CameraManipulatorType::eOrbit);
    EXPECT_GT(manip->getZoomSpeed(), 0.0f);
}

TEST(ApiRobustness, SetZoomSpeedReflectedViaInterface) {
    vne::interaction::OrbitManipulator manip;
    manip.setZoomSpeed(1.5f);
    // Access via interface pointer to confirm virtual dispatch
    vne::interaction::ICameraManipulator* iface = &manip;
    EXPECT_FLOAT_EQ(iface->getZoomSpeed(), 1.5f);
}

// ---------------------------------------------------------------------------
// Scroll zoom uses manipulator's zoom_speed (adapter path consistency)
// ---------------------------------------------------------------------------

TEST(ApiRobustness, ScrollZoomViaAdapterUsesManipulatorZoomSpeed) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->updateMatrices();

    auto manip = std::make_shared<vne::interaction::OrbitManipulator>();
    manip->setCamera(cam);
    manip->setViewportSize(1280.0f, 720.0f);
    const float fast_speed = 2.0f;
    manip->setZoomSpeed(fast_speed);

    vne::interaction::CameraSystemController ctrl;
    ctrl.setCamera(cam);
    ctrl.setManipulator(manip);
    ctrl.setViewportSize(1280.0f, 720.0f);

    const float dist_before = manip->getOrbitDistance();
    // One scroll-in notch via the adapter (should use zoom_speed = 2.0)
    ctrl.handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, 0.016);

    const float dist_after = manip->getOrbitDistance();
    // With zoom_speed=2.0, distance shrinks to dist/2.0; with default 1.1 it shrinks to dist/1.1
    // Verify the change is consistent with fast_speed, not the old hardcoded 1.1
    const float ratio = dist_before / dist_after;
    EXPECT_NEAR(ratio, fast_speed, 0.05f) << "Adapter scroll should use manipulator's zoom_speed, not hardcoded 1.1";
}

// ---------------------------------------------------------------------------
// Orbit: rotation actually moves the camera
// ---------------------------------------------------------------------------

TEST(ApiRobustness, OrbitRotationMovesCamera) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    const vne::math::Vec3f pos_before = cam->getPosition();
    // Press left button, drag horizontally by 100px, release
    manip.handleMouseButton(0, true, 640.0f, 360.0f, 0.016);
    manip.handleMouseMove(740.0f, 360.0f, 100.0f, 0.0f, 0.016);
    manip.handleMouseButton(0, false, 740.0f, 360.0f, 0.016);

    const vne::math::Vec3f pos_after = cam->getPosition();
    EXPECT_GT((pos_after - pos_before).length(), 0.01f) << "Orbit drag should move camera position";
}

TEST(ApiRobustness, OrbitRotationPreservesDistance) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    const float dist_before = manip.getOrbitDistance();
    manip.handleMouseButton(0, true, 640.0f, 360.0f, 0.016);
    manip.handleMouseMove(740.0f, 400.0f, 100.0f, 40.0f, 0.016);
    manip.handleMouseButton(0, false, 740.0f, 400.0f, 0.016);

    // Distance from eye to COI must remain constant after rotation
    const vne::math::Vec3f eye = cam->getPosition();
    const vne::math::Vec3f coi = manip.getCenterOfInterestWorld();
    EXPECT_NEAR((eye - coi).length(), dist_before, 1e-3f) << "Orbit rotation must preserve orbit distance";
}

// ---------------------------------------------------------------------------
// Orbit: scroll zoom changes orbit distance
// ---------------------------------------------------------------------------

TEST(ApiRobustness, ScrollZoomInDecreasesOrbitDistance) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    const float dist_before = manip.getOrbitDistance();
    manip.handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, 0.016);  // scroll up = zoom in
    EXPECT_LT(manip.getOrbitDistance(), dist_before) << "Scroll up should decrease orbit distance (zoom in)";
}

TEST(ApiRobustness, ScrollZoomOutIncreasesOrbitDistance) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    const float dist_before = manip.getOrbitDistance();
    manip.handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, 0.016);  // scroll down = zoom out
    EXPECT_GT(manip.getOrbitDistance(), dist_before) << "Scroll down should increase orbit distance (zoom out)";
}

// ---------------------------------------------------------------------------
// Orbit: pan moves center of interest
// ---------------------------------------------------------------------------

TEST(ApiRobustness, OrbitPanMovesCenterOfInterest) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    const vne::math::Vec3f coi_before = manip.getCenterOfInterestWorld();
    // Middle-mouse pan
    manip.handleMouseButton(2, true, 640.0f, 360.0f, 0.016);
    manip.handleMouseMove(740.0f, 360.0f, 100.0f, 0.0f, 0.016);
    manip.handleMouseButton(2, false, 740.0f, 360.0f, 0.016);

    const vne::math::Vec3f coi_after = manip.getCenterOfInterestWorld();
    EXPECT_GT((coi_after - coi_before).length(), 1e-4f) << "Middle-mouse pan must move center of interest";
}

// ---------------------------------------------------------------------------
// FPS: keyboard movement moves camera
// ---------------------------------------------------------------------------

TEST(ApiRobustness, FpsKeyboardForwardMovesCamera) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, -1.0f));
    cam->updateMatrices();

    vne::interaction::FpsManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);
    manip.setMoveSpeed(5.0f);

    const vne::math::Vec3f pos_before = cam->getPosition();
    manip.handleKeyboard(87, true, 0.016);   // W pressed
    manip.update(0.1);                       // 100ms of movement
    manip.handleKeyboard(87, false, 0.016);  // W released

    const vne::math::Vec3f pos_after = cam->getPosition();
    EXPECT_GT((pos_after - pos_before).length(), 0.01f) << "W key held for 100ms must move FPS camera forward";
}

TEST(ApiRobustness, FpsSprintMultiplierMovesMoreThanNormal) {
    auto cam1 = makePerspCamera();
    cam1->setPosition(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam1->setTarget(vne::math::Vec3f(0.0f, 0.0f, -1.0f));
    cam1->updateMatrices();

    auto cam2 = makePerspCamera();
    cam2->setPosition(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam2->setTarget(vne::math::Vec3f(0.0f, 0.0f, -1.0f));
    cam2->updateMatrices();

    vne::interaction::FpsManipulator normal_manip;
    normal_manip.setCamera(cam1);
    normal_manip.setViewportSize(1280.0f, 720.0f);
    normal_manip.handleKeyboard(87, true, 0.016);
    normal_manip.update(0.1);

    vne::interaction::FpsManipulator sprint_manip;
    sprint_manip.setCamera(cam2);
    sprint_manip.setViewportSize(1280.0f, 720.0f);
    sprint_manip.handleKeyboard(340, true, 0.016);  // Shift (sprint)
    sprint_manip.handleKeyboard(87, true, 0.016);   // W
    sprint_manip.update(0.1);

    const float normal_dist = (cam1->getPosition() - vne::math::Vec3f(0.0f)).length();
    const float sprint_dist = (cam2->getPosition() - vne::math::Vec3f(0.0f)).length();
    EXPECT_GT(sprint_dist, normal_dist) << "Sprint+W must move camera farther than W alone in same time";
}

// ---------------------------------------------------------------------------
// FitToAABB: camera actually frames the bounding box
// ---------------------------------------------------------------------------

TEST(ApiRobustness, OrbitFitToAABBPositionsCameraToSeeEntireBox) {
    auto cam = makePerspCamera();
    vne::interaction::OrbitManipulator manip;
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    const vne::math::Vec3f min_w{-10.0f, -10.0f, -10.0f};
    const vne::math::Vec3f max_w{10.0f, 10.0f, 10.0f};
    manip.fitToAABB(min_w, max_w);

    // COI must be at box center
    const vne::math::Vec3f center = (min_w + max_w) * 0.5f;
    const vne::math::Vec3f coi = manip.getCenterOfInterestWorld();
    EXPECT_NEAR(coi.x(), center.x(), 1e-3f);
    EXPECT_NEAR(coi.y(), center.y(), 1e-3f);
    EXPECT_NEAR(coi.z(), center.z(), 1e-3f);

    // Orbit distance must be large enough to see the box (radius ~17.3 units)
    EXPECT_GT(manip.getOrbitDistance(), 10.0f) << "fitToAABB should set orbit distance large enough to frame the scene";
}

// ---------------------------------------------------------------------------
// Ortho pan zoom: scroll zoom changes ortho extents
// ---------------------------------------------------------------------------

TEST(ApiRobustness, OrthoPanZoomScrollChangesExtents) {
    auto ortho = makeOrthoCamera();
    vne::interaction::OrthoPanZoomManipulator manip;
    manip.setCamera(ortho);
    manip.setViewportSize(1280.0f, 720.0f);

    const float extent_before = ortho->getWidth();
    manip.handleMouseScroll(0.0f, 1.0f, 640.0f, 360.0f, 0.016);  // zoom in
    EXPECT_LT(ortho->getWidth(), extent_before) << "Scroll in on ortho manipulator must shrink ortho extents";
}

// ---------------------------------------------------------------------------
// ICameraBehavior interface — FollowManipulator satisfies it
// ---------------------------------------------------------------------------

TEST(ApiRobustness, FollowManipulatorImplementsICameraBehavior) {
    auto follow = std::make_shared<vne::interaction::FollowManipulator>();
    // Cast to ICameraBehavior must succeed
    auto behavior = std::dynamic_pointer_cast<vne::interaction::ICameraBehavior>(follow);
    ASSERT_NE(behavior, nullptr) << "FollowManipulator must implement ICameraBehavior";
}

TEST(ApiRobustness, ICameraBehaviorSetEnabledIsEnabledConsistent) {
    vne::interaction::FollowManipulator follow;
    vne::interaction::ICameraBehavior* behavior = &follow;

    EXPECT_TRUE(behavior->isEnabled());
    behavior->setEnabled(false);
    EXPECT_FALSE(behavior->isEnabled());
    behavior->setEnabled(true);
    EXPECT_TRUE(behavior->isEnabled());
}

TEST(ApiRobustness, ICameraBehaviorResetStateDoesNotCrash) {
    auto cam = makePerspCamera();
    vne::interaction::FollowManipulator follow;
    follow.setCamera(cam);
    follow.setTargetWorld({1.0f, 2.0f, 3.0f});
    vne::interaction::ICameraBehavior* behavior = &follow;
    EXPECT_NO_FATAL_FAILURE(behavior->resetState());
}

// ---------------------------------------------------------------------------
// Follow: camera moves toward target over time
// ---------------------------------------------------------------------------

TEST(ApiRobustness, FollowManipulatorMovesCamera) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->updateMatrices();

    vne::interaction::FollowManipulator manip;
    manip.setCamera(cam);
    manip.setTargetWorld(vne::math::Vec3f(100.0f, 0.0f, 0.0f));
    manip.setOffsetWorld(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    manip.setDamping(50.0f);  // high damping → fast convergence in test

    const vne::math::Vec3f pos_before = cam->getPosition();
    manip.update(0.1);
    const vne::math::Vec3f pos_after = cam->getPosition();

    EXPECT_GT((pos_after - pos_before).length(), 0.1f) << "FollowManipulator update must move camera toward target";
}

// ---------------------------------------------------------------------------
// toWorldUp helper
// ---------------------------------------------------------------------------

TEST(ApiRobustness, ToWorldUpYReturnsYAxis) {
    const vne::math::Vec3f up = vne::interaction::toWorldUp(vne::interaction::UpAxis::eY);
    EXPECT_FLOAT_EQ(up.x(), 0.0f);
    EXPECT_FLOAT_EQ(up.y(), 1.0f);
    EXPECT_FLOAT_EQ(up.z(), 0.0f);
}

TEST(ApiRobustness, ToWorldUpZReturnsZAxis) {
    const vne::math::Vec3f up = vne::interaction::toWorldUp(vne::interaction::UpAxis::eZ);
    EXPECT_FLOAT_EQ(up.x(), 0.0f);
    EXPECT_FLOAT_EQ(up.y(), 0.0f);
    EXPECT_FLOAT_EQ(up.z(), 1.0f);
}

// ---------------------------------------------------------------------------
// Z-up orbit: rotation stays on horizontal plane
// ---------------------------------------------------------------------------

TEST(ApiRobustness, OrbitZUpRotationStaysAtSameHeight) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(5.0f, 0.0f, 0.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    cam->setUp(vne::math::Vec3f(0.0f, 0.0f, 1.0f));
    cam->updateMatrices();

    vne::interaction::OrbitManipulator manip;
    manip.setWorldUp(vne::interaction::toWorldUp(vne::interaction::UpAxis::eZ));
    manip.setCamera(cam);
    manip.setViewportSize(1280.0f, 720.0f);

    // Pure horizontal drag (no vertical) must not change height (Z component) when Z is up
    manip.handleMouseButton(0, true, 640.0f, 360.0f, 0.016);
    manip.handleMouseMove(740.0f, 360.0f, 100.0f, 0.0f, 0.016);  // pure horizontal
    manip.handleMouseButton(0, false, 740.0f, 360.0f, 0.016);

    EXPECT_NEAR(cam->getPosition().z(), 0.0f, 0.1f) << "Pure horizontal drag with Z-up must not change camera height";
}

}  // namespace vne_interaction_test
