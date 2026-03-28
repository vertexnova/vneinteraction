/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

/**
 * Controllers store InputMapper callbacks that must stay valid after std::move
 * (Impl* capture, not [this]). These tests construct, move, then drive onEvent /
 * onUpdate so actions still reach the CameraRig without dangling captures.
 */

#include "vertexnova/interaction/follow_controller.h"
#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/interaction/ortho_2d_controller.h"

#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"

#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

namespace vne_interaction_test {

namespace {

std::shared_ptr<vne::scene::PerspectiveCamera> makePerspCamera() {
    return vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
}

std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
}

}  // namespace

TEST(ControllerMoveSafety, Inspect3DController_EventAndUpdateAfterMove) {
    vne::interaction::Inspect3DController src;
    auto cam = makePerspCamera();
    src.setCamera(cam);
    src.onResize(1280.0f, 720.0f);

    vne::interaction::Inspect3DController moved = std::move(src);

    vne::events::MouseScrolledEvent scroll(0.0, 1.0);
    vne::events::MouseButtonPressedEvent press(vne::events::MouseButton::eLeft, 0, 640.0, 360.0);
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(scroll, 0.016));
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(press, 0.016));
    EXPECT_NO_FATAL_FAILURE(moved.onUpdate(0.016));
}

TEST(ControllerMoveSafety, Navigation3DController_EventAndUpdateAfterMove) {
    vne::interaction::Navigation3DController src;
    auto cam = makePerspCamera();
    src.setCamera(cam);
    src.onResize(1280.0f, 720.0f);

    vne::interaction::Navigation3DController moved = std::move(src);

    vne::events::KeyPressedEvent w(vne::events::KeyCode::eW);
    vne::events::MouseButtonPressedEvent look(vne::events::MouseButton::eRight, 0, 640.0, 360.0);
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(w, 0.016));
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(look, 0.016));
    EXPECT_NO_FATAL_FAILURE(moved.onUpdate(0.016));
}

TEST(ControllerMoveSafety, Ortho2DController_EventAndUpdateAfterMove) {
    vne::interaction::Ortho2DController src;
    auto cam = makeOrthoCamera();
    src.setCamera(cam);
    src.onResize(512.0f, 512.0f);

    vne::interaction::Ortho2DController moved = std::move(src);

    vne::events::MouseMovedEvent move(256.0, 256.0);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(move));
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(scroll));
    EXPECT_NO_FATAL_FAILURE(moved.onUpdate(0.016));
}

TEST(ControllerMoveSafety, FollowController_EventAndUpdateAfterMove) {
    vne::interaction::FollowController src;
    auto cam = makePerspCamera();
    src.setCamera(cam);
    src.onResize(1280.0f, 720.0f);
    const vne::math::Mat4f target = vne::math::Mat4f::translate(vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    src.setTarget(target);

    vne::interaction::FollowController moved = std::move(src);

    vne::events::MouseScrolledEvent scroll(0.0, 1.0);
    EXPECT_NO_FATAL_FAILURE(moved.onEvent(scroll));
    EXPECT_NO_FATAL_FAILURE(moved.onUpdate(0.016));
}

}  // namespace vne_interaction_test
