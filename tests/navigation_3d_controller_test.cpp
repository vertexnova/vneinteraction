/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <memory>

#include <gtest/gtest.h>

namespace vne_interaction_test {

namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr float kVpW = 1280.0f;
constexpr float kVpH = 720.0f;
constexpr float kCx = kVpW * 0.5f;
constexpr float kCy = kVpH * 0.5f;

std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

/** Default pose: eye (0,0,5) looking at origin, Y-up — matches regression / examples. */
std::shared_ptr<vne::scene::PerspectiveCamera> makeNav3dTestCamera() {
    auto cam = makePerspCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();
    return cam;
}

vne::math::Vec3f forwardFromCamera(const vne::scene::PerspectiveCamera& cam) {
    const vne::math::Vec3f eye = cam.getPosition();
    const vne::math::Vec3f tgt = cam.getTarget();
    const vne::math::Vec3f d = tgt - eye;
    const float len = d.length();
    return (len >= 1e-5f) ? (d / len) : cam.getForward();
}

struct Nav3dIntegrationSetup {
    vne::interaction::Navigation3DController ctrl;
    std::shared_ptr<vne::scene::PerspectiveCamera> cam;

    Nav3dIntegrationSetup() {
        cam = makeNav3dTestCamera();
        ctrl.setCamera(cam);
        ctrl.onResize(kVpW, kVpH);
        ctrl.setMouseSensitivity(1.0f);
        ctrl.setMoveSpeed(10.0f);
    }
};

void pressKey(vne::interaction::Navigation3DController& ctrl, vne::events::KeyCode k) {
    ctrl.onEvent(vne::events::KeyPressedEvent(k), kDt);
}

void releaseKey(vne::interaction::Navigation3DController& ctrl, vne::events::KeyCode k) {
    ctrl.onEvent(vne::events::KeyReleasedEvent(k), kDt);
}

}  // namespace

TEST(Navigation3DController, DefaultFpsMode) {
    vne::interaction::Navigation3DController ctrl;
    EXPECT_EQ(ctrl.getMode(), vne::interaction::FreeLookMode::eFps);
}

TEST(Navigation3DController, DefaultYawPitchRotationMode) {
    vne::interaction::Navigation3DController ctrl;
    EXPECT_EQ(ctrl.getRotationMode(), vne::interaction::FreeLookRotationMode::eYawPitch);
}

TEST(Navigation3DController, SetRotationModeTrackball) {
    vne::interaction::Navigation3DController ctrl;
    ctrl.setRotationMode(vne::interaction::FreeLookRotationMode::eTrackball);
    EXPECT_EQ(ctrl.getRotationMode(), vne::interaction::FreeLookRotationMode::eTrackball);
    EXPECT_EQ(ctrl.freeLookManipulator().getRotationMode(), vne::interaction::FreeLookRotationMode::eTrackball);
}

TEST(Navigation3DController, SetMode) {
    vne::interaction::Navigation3DController ctrl;
    ctrl.setMode(vne::interaction::FreeLookMode::eFly);
    EXPECT_EQ(ctrl.getMode(), vne::interaction::FreeLookMode::eFly);
    ctrl.setMode(vne::interaction::FreeLookMode::eFps);
    EXPECT_EQ(ctrl.getMode(), vne::interaction::FreeLookMode::eFps);
}

TEST(Navigation3DController, FreeLookManipulatorEscapeHatchStableAcrossRebuild) {
    vne::interaction::Navigation3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    vne::interaction::FreeLookManipulator& fl = ctrl.freeLookManipulator();
    ctrl.setMoveSpeed(7.0f);
    EXPECT_FLOAT_EQ(fl.getMoveSpeed(), 7.0f);

    ctrl.setMode(vne::interaction::FreeLookMode::eFly);
    EXPECT_EQ(&ctrl.freeLookManipulator(), &fl);

    ctrl.freeLookManipulator().setMouseSensitivity(0.22f);
    EXPECT_FLOAT_EQ(fl.getMouseSensitivity(), 0.22f);
}

TEST(Navigation3DController, OnEventNoCrash) {
    vne::interaction::Navigation3DController ctrl;
    auto cam = makePerspCamera();
    ctrl.setCamera(cam);
    ctrl.onResize(1280.0f, 720.0f);

    vne::events::KeyPressedEvent press(vne::events::KeyCode::eW);
    vne::events::MouseButtonPressedEvent look(vne::events::MouseButton::eRight, 0, 640.0, 360.0);

    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(press, 0.016));
    EXPECT_NO_FATAL_FAILURE(ctrl.onEvent(look, 0.016));
}

// ---------------------------------------------------------------------------
// Integration: RMB + mouse move → look (yaw / pitch) via real vne::events
// FreeLook: yaw += delta_x * sens, pitch -= delta_y * sens (see free_look_manipulator.cpp).
// ---------------------------------------------------------------------------

TEST(Navigation3DIntegration, MouseLeftWhileLooking_DecreasesForwardX) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f fwd_before = forwardFromCamera(*s.cam);

    s.ctrl.onEvent(vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eRight,
                                                        0,
                                                        static_cast<double>(kCx),
                                                        static_cast<double>(kCy)),
                   kDt);
    s.ctrl.onEvent(vne::events::MouseMovedEvent(static_cast<double>(kCx - 80.0f), static_cast<double>(kCy)), kDt);
    s.ctrl.onEvent(vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eRight,
                                                         0,
                                                         static_cast<double>(kCx - 80.0f),
                                                         static_cast<double>(kCy)),
                   kDt);

    const vne::math::Vec3f fwd_after = forwardFromCamera(*s.cam);
    EXPECT_LT(fwd_after.x(), fwd_before.x())
        << "Mouse left (negative dx) decreases yaw; forward X should decrease from ~0 for +Z eye on COI";
}

TEST(Navigation3DIntegration, MouseRightWhileLooking_IncreasesForwardX) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f fwd_before = forwardFromCamera(*s.cam);

    s.ctrl.onEvent(vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eRight,
                                                        0,
                                                        static_cast<double>(kCx),
                                                        static_cast<double>(kCy)),
                   kDt);
    s.ctrl.onEvent(vne::events::MouseMovedEvent(static_cast<double>(kCx + 80.0f), static_cast<double>(kCy)), kDt);
    s.ctrl.onEvent(vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eRight,
                                                         0,
                                                         static_cast<double>(kCx + 80.0f),
                                                         static_cast<double>(kCy)),
                   kDt);

    const vne::math::Vec3f fwd_after = forwardFromCamera(*s.cam);
    EXPECT_GT(fwd_after.x(), fwd_before.x())
        << "Mouse right (positive dx) increases yaw; forward X should increase from ~0 for +Z eye on COI";
}

TEST(Navigation3DIntegration, MouseDownWhileLooking_DecreasesForwardY) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f fwd_before = forwardFromCamera(*s.cam);

    s.ctrl.onEvent(vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eRight,
                                                        0,
                                                        static_cast<double>(kCx),
                                                        static_cast<double>(kCy)),
                   kDt);
    s.ctrl.onEvent(vne::events::MouseMovedEvent(static_cast<double>(kCx), static_cast<double>(kCy + 50.0f)), kDt);
    s.ctrl.onEvent(vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eRight,
                                                         0,
                                                         static_cast<double>(kCx),
                                                         static_cast<double>(kCy + 50.0f)),
                   kDt);

    const vne::math::Vec3f fwd_after = forwardFromCamera(*s.cam);
    EXPECT_LT(fwd_after.y(), fwd_before.y())
        << "Mouse down (positive dy) reduces pitch; forward Y should drop vs level look-at-origin";
}

// ---------------------------------------------------------------------------
// Integration: WASD + Q/E + onUpdate
// ---------------------------------------------------------------------------

TEST(Navigation3DIntegration, KeyW_MovesAlongForward) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f pos0 = s.cam->getPosition();
    const vne::math::Vec3f f0 = s.cam->getForward().normalized();

    pressKey(s.ctrl, vne::events::KeyCode::eW);
    for (int i = 0; i < 20; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eW);

    const vne::math::Vec3f delta = s.cam->getPosition() - pos0;
    EXPECT_GT(delta.dot(f0), 0.01f) << "W should translate along +camera forward";
}

TEST(Navigation3DIntegration, KeyS_MovesOppositeForward) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f pos0 = s.cam->getPosition();
    const vne::math::Vec3f f0 = s.cam->getForward().normalized();

    pressKey(s.ctrl, vne::events::KeyCode::eS);
    for (int i = 0; i < 20; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eS);

    const vne::math::Vec3f delta = s.cam->getPosition() - pos0;
    EXPECT_LT(delta.dot(f0), -0.01f) << "S should translate opposite camera forward";
}

TEST(Navigation3DIntegration, KeyD_MovesAlongRight) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f pos0 = s.cam->getPosition();
    const vne::math::Vec3f r0 = s.cam->getRight().normalized();

    pressKey(s.ctrl, vne::events::KeyCode::eD);
    for (int i = 0; i < 20; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eD);

    const vne::math::Vec3f delta = s.cam->getPosition() - pos0;
    EXPECT_GT(delta.dot(r0), 0.01f) << "D should strafe along +camera right";
}

TEST(Navigation3DIntegration, KeyA_MovesOppositeRight) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f pos0 = s.cam->getPosition();
    const vne::math::Vec3f r0 = s.cam->getRight().normalized();

    pressKey(s.ctrl, vne::events::KeyCode::eA);
    for (int i = 0; i < 20; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eA);

    const vne::math::Vec3f delta = s.cam->getPosition() - pos0;
    EXPECT_LT(delta.dot(r0), -0.01f) << "A should strafe along -camera right";
}

TEST(Navigation3DIntegration, KeyE_MovesWorldUpInFps) {
    Nav3dIntegrationSetup s;
    EXPECT_EQ(s.ctrl.getMode(), vne::interaction::FreeLookMode::eFps);
    const vne::math::Vec3f pos0 = s.cam->getPosition();
    const vne::math::Vec3f world_up(0.0f, 1.0f, 0.0f);

    pressKey(s.ctrl, vne::events::KeyCode::eE);
    for (int i = 0; i < 20; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eE);

    const vne::math::Vec3f delta = s.cam->getPosition() - pos0;
    EXPECT_GT(delta.dot(world_up), 0.01f) << "E (move up) should add +world Y in FPS with level camera";
}

TEST(Navigation3DIntegration, KeyQ_MovesWorldDownInFps) {
    Nav3dIntegrationSetup s;
    const vne::math::Vec3f pos0 = s.cam->getPosition();
    const vne::math::Vec3f world_up(0.0f, 1.0f, 0.0f);

    pressKey(s.ctrl, vne::events::KeyCode::eQ);
    for (int i = 0; i < 20; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eQ);

    const vne::math::Vec3f delta = s.cam->getPosition() - pos0;
    EXPECT_LT(delta.dot(world_up), -0.01f) << "Q (move down) should add -world Y in FPS with level camera";
}

// ---------------------------------------------------------------------------
// Optional: DOF gating + scroll zoom (default rules)
// ---------------------------------------------------------------------------

TEST(Navigation3DIntegration, SetMoveEnabledFalse_IgnoresW) {
    Nav3dIntegrationSetup s;
    s.ctrl.setMoveEnabled(false);
    const vne::math::Vec3f pos0 = s.cam->getPosition();

    pressKey(s.ctrl, vne::events::KeyCode::eW);
    for (int i = 0; i < 15; ++i)
        s.ctrl.onUpdate(kDt);
    releaseKey(s.ctrl, vne::events::KeyCode::eW);

    EXPECT_NEAR((s.cam->getPosition() - pos0).length(), 0.0f, 1e-3f);
}

TEST(Navigation3DIntegration, SetLookEnabledFalse_IgnoresMouseLook) {
    Nav3dIntegrationSetup s;
    s.ctrl.setLookEnabled(false);
    const vne::math::Vec3f fwd_before = forwardFromCamera(*s.cam);

    s.ctrl.onEvent(vne::events::MouseButtonPressedEvent(vne::events::MouseButton::eRight,
                                                        0,
                                                        static_cast<double>(kCx),
                                                        static_cast<double>(kCy)),
                   kDt);
    s.ctrl.onEvent(vne::events::MouseMovedEvent(static_cast<double>(kCx + 120.0f), static_cast<double>(kCy)), kDt);
    s.ctrl.onEvent(vne::events::MouseButtonReleasedEvent(vne::events::MouseButton::eRight,
                                                         0,
                                                         static_cast<double>(kCx + 120.0f),
                                                         static_cast<double>(kCy)),
                   kDt);

    const vne::math::Vec3f fwd_after = forwardFromCamera(*s.cam);
    EXPECT_NEAR((fwd_after - fwd_before).length(), 0.0f, 1e-4f);
}

TEST(Navigation3DIntegration, MouseScroll_AppliesZoomWhenEnabled) {
    Nav3dIntegrationSetup s;
    ASSERT_TRUE(s.ctrl.freeLookManipulator().getHandleZoom());
    ASSERT_EQ(s.ctrl.freeLookManipulator().getZoomMethod(), vne::interaction::ZoomMethod::eSceneScale);

    const float scale_before = s.cam->getSceneScale();
    s.ctrl.onEvent(vne::events::MouseScrolledEvent(0.0, -1.0), kDt);
    s.ctrl.onUpdate(kDt);

    EXPECT_NE(s.cam->getSceneScale(), scale_before)
        << "Default FreeLook zoom is scene-scale; scroll should change ICamera::getSceneScale()";
}

}  // namespace vne_interaction_test
