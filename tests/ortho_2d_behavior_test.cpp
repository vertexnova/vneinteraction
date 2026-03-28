/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/ortho_2d_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
}

TEST(Ortho2DBehavior, DefaultValues) {
    vne::interaction::Ortho2DBehavior b;
    EXPECT_GT(b.getZoomSpeed(), 0.0f);
}

TEST(Ortho2DBehavior, CameraIntegration) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));

    vne::interaction::Ortho2DBehavior b;
    b.setCamera(cam);
    b.onResize(512.0f, 512.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 256.0f;
    p.y_px = 256.0f;
    p.delta_x_px = 50.0f;
    p.delta_y_px = 30.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginPan, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::ePanDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndPan, p, 0.016);
    b.onUpdate(0.016);

    EXPECT_GT((cam->getPosition() - vne::math::Vec3f(0.0f, 0.0f, 5.0f)).length(), 0.001f);
}

TEST(Ortho2DBehavior, ZoomAtCursor) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));

    vne::interaction::Ortho2DBehavior b;
    b.setCamera(cam);
    b.onResize(512.0f, 512.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 256.0f;
    p.y_px = 256.0f;
    p.zoom_factor = 1.2f;

    std::ignore = b.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, 0.016);
    b.onUpdate(0.016);

    EXPECT_GT(b.getWorldUnitsPerPixel(), 0.0f);
}

TEST(Ortho2DBehavior, InPlaneRotatePreservesEyeTargetDistance) {
    auto cam = makeOrthoCamera();
    cam->lookAt(vne::math::Vec3f(0.0f, 0.0f, 10.0f),
                vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    cam->updateMatrices();

    vne::interaction::Ortho2DBehavior b;
    b.setCamera(cam);
    b.onResize(512.0f, 512.0f);

    const float dist_before = (cam->getPosition() - cam->getTarget()).length();
    const vne::math::Vec3f up_before = cam->getUp();

    vne::interaction::CameraCommandPayload p;
    p.delta_x_px = 80.0f;
    p.delta_y_px = 0.0f;

    b.onAction(vne::interaction::CameraActionType::eBeginRotate, p, 0.0);
    b.onAction(vne::interaction::CameraActionType::eRotateDelta, p, 0.016);
    b.onAction(vne::interaction::CameraActionType::eEndRotate, p, 0.0);

    const float dist_after = (cam->getPosition() - cam->getTarget()).length();
    EXPECT_NEAR(dist_before, dist_after, 1e-4f);
    EXPECT_GT((cam->getUp() - up_before).length(), 0.01f);
}

}  // namespace vne_interaction_test
