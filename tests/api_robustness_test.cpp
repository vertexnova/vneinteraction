/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Behavioral correctness and API robustness tests for the camera manipulator system.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_controller.h"
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"
#include "vertexnova/interaction/ortho_2d_controller.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include "vertexnova/events/mouse_event.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>
#include <memory>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

static std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
}

TEST(ApiRobustness, OrbitalCameraManipulatorIsEnabledDefaultTrue) {
    vne::interaction::OrbitalCameraManipulator b;
    EXPECT_TRUE(b.isEnabled());
}

TEST(ApiRobustness, OrbitalCameraManipulatorSetEnabledFalseReflected) {
    vne::interaction::OrbitalCameraManipulator b;
    b.setEnabled(false);
    EXPECT_FALSE(b.isEnabled());
}

TEST(ApiRobustness, DisabledOrbitalCameraManipulatorDoesNotMoveCamera) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::OrbitalCameraManipulator b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setEnabled(false);

    const auto pos_before = cam->getPosition();
    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.delta_x_px = 100.0f;
    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.016);
    b.onUpdate(0.016);

    const auto pos_after = cam->getPosition();
    EXPECT_NEAR((pos_after - pos_before).length(), 0.0f, 1e-4f);
}

TEST(ApiRobustness, SetViewportSizeZeroClamped) {
    vne::interaction::OrbitalCameraManipulator b;
    auto cam = makePerspCamera();
    b.setCamera(cam);
    b.onResize(0.0f, 0.0f);
    EXPECT_GT(b.getWorldUnitsPerPixel(), 0.0f);
    EXPECT_TRUE(std::isfinite(b.getWorldUnitsPerPixel()));
}

// setCamera(nullptr) is documented as valid (detach). After detach, onUpdate/onAction must not crash.
TEST(ApiRobustness, DetachThenUpdateAndActionSafe) {
    auto cam = makePerspCamera();
    vne::interaction::CameraCommandPayload p;
    p.x_px = 640.0f;
    p.y_px = 360.0f;
    p.delta_x_px = 10.0f;
    p.zoom_factor = 1.1f;

    vne::interaction::OrbitalCameraManipulator b;
    b.setCamera(cam);
    b.onResize(1280.0f, 720.0f);
    b.setCamera(nullptr);  // detach

    EXPECT_NO_THROW({
        b.onResize(100.0f, 100.0f);
        b.onUpdate(0.016);
        b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.016);
        b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
        b.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, 0.016);
        b.onUpdate(0.016);
    });
}

TEST(ApiRobustness, ControllerDetachThenEventAndUpdateSafe) {
    auto cam = makePerspCamera();
    vne::interaction::Inspect3DController ctrl;
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);
    ctrl.setCamera(nullptr);  // detach

    EXPECT_NO_THROW({
        ctrl.onResize(100.0f, 100.0f);
        vne::events::MouseMovedEvent move(50.0, 50.0);
        ctrl.onEvent(move, 0.016);
        vne::events::MouseScrolledEvent scroll(0.0, 1.0);
        ctrl.onEvent(scroll, 0.016);
        ctrl.onUpdate(0.016);
    });
}

TEST(ApiRobustness, Inspect3DControllerScrollZoom) {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 10.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    vne::interaction::Inspect3DController ctrl;
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);
    ctrl.orbitalCameraManipulator().setZoomSpeed(2.0f);
    ctrl.orbitalCameraManipulator().setZoomMethod(vne::interaction::ZoomMethod::eDollyToCoi);

    const float dist_before = ctrl.orbitalCameraManipulator().getOrbitDistance();
    vne::events::MouseMovedEvent pos(640.0, 360.0);
    ctrl.onEvent(pos, 0.016);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);
    ctrl.onEvent(scroll, 0.016);
    ctrl.onUpdate(0.016);

    const float dist_after = ctrl.orbitalCameraManipulator().getOrbitDistance();
    EXPECT_LT(dist_after, dist_before);
}

TEST(ApiRobustness, FitToAABBPositionsCOI) {
    auto cam = makePerspCamera();
    vne::interaction::Inspect3DController ctrl;
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    const vne::math::Vec3f min_w(-10.0f, -10.0f, -10.0f);
    const vne::math::Vec3f max_w(10.0f, 10.0f, 10.0f);
    ctrl.fitToAABB(min_w, max_w);

    const auto coi = ctrl.orbitalCameraManipulator().getCenterOfInterestWorld();
    const vne::math::Vec3f center = (min_w + max_w) * 0.5f;
    EXPECT_NEAR(coi.x(), center.x(), 1e-2f);
    EXPECT_NEAR(coi.y(), center.y(), 1e-2f);
    EXPECT_NEAR(coi.z(), center.z(), 1e-2f);
}

TEST(ApiRobustness, ICameraControllerPolymorphicSmoke) {
    std::unique_ptr<vne::interaction::ICameraController> c = std::make_unique<vne::interaction::Inspect3DController>();
    c->setCamera(makePerspCamera());
    c->onResize(800.0f, 600.0f);
    vne::events::MouseMovedEvent move(100.0, 100.0);
    EXPECT_NO_FATAL_FAILURE(c->onEvent(move, 0.016));
    EXPECT_NO_FATAL_FAILURE(c->onUpdate(0.016));
}

TEST(ApiRobustness, Ortho2DControllerResetClearsMapperState) {
    auto ortho = makeOrthoCamera();
    vne::interaction::Ortho2DController ctrl;
    ctrl.setCamera(ortho);
    ctrl.onResize(512.0f, 512.0f);
    vne::events::MouseButtonPressedEvent press(vne::events::MouseButton::eRight, 0, 128.0, 128.0);
    ctrl.onEvent(press, 0.016);
    ctrl.reset();
    vne::events::MouseMovedEvent move(200.0, 200.0);
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(move, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onUpdate(0.016));
}

TEST(ApiRobustness, Ortho2DControllerScrollChangesExtents) {
    auto ortho = makeOrthoCamera();
    vne::interaction::Ortho2DController ctrl;
    ctrl.setCamera(ortho);
    ctrl.onResize(1280.0f, 720.0f);
    ctrl.ortho2DManipulator().setZoomMethod(vne::interaction::ZoomMethod::eDollyToCoi);

    const float extent_before = ortho->getWidth();
    vne::events::MouseMovedEvent pos(640.0, 360.0);
    ctrl.onEvent(pos);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);
    ctrl.onEvent(scroll);
    ctrl.onUpdate(0.016);

    EXPECT_LT(ortho->getWidth(), extent_before);
}

TEST(ApiRobustness, ToWorldUpYReturnsYAxis) {
    const auto up = vne::interaction::toWorldUp(vne::interaction::UpAxis::eY);
    EXPECT_FLOAT_EQ(up.x(), 0.0f);
    EXPECT_FLOAT_EQ(up.y(), 1.0f);
    EXPECT_FLOAT_EQ(up.z(), 0.0f);
}

TEST(ApiRobustness, ToWorldUpZReturnsZAxis) {
    const auto up = vne::interaction::toWorldUp(vne::interaction::UpAxis::eZ);
    EXPECT_FLOAT_EQ(up.x(), 0.0f);
    EXPECT_FLOAT_EQ(up.y(), 0.0f);
    EXPECT_FLOAT_EQ(up.z(), 1.0f);
}

}  // namespace vne_interaction_test
