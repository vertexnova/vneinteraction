/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/pan_zoom_behavior.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

static std::shared_ptr<vne::scene::OrthographicCamera> makeOrthoCamera() {
    return vne::scene::CameraFactory::createOrthographic(
        vne::scene::OrthographicCameraParameters(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f));
}

TEST(PanZoomBehavior, DefaultValues) {
    vne::interaction::PanZoomBehavior b;
    EXPECT_GT(b.getZoomSpeed(), 0.0f);
}

TEST(PanZoomBehavior, CameraIntegration) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));

    vne::interaction::PanZoomBehavior b;
    b.setCamera(cam);
    b.setViewportSize(512.0f, 512.0f);

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

TEST(PanZoomBehavior, ZoomAtCursor) {
    auto cam = makeOrthoCamera();
    cam->setPosition(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    cam->setTarget(vne::math::Vec3f(0.0f, 0.0f, 0.0f));

    vne::interaction::PanZoomBehavior b;
    b.setCamera(cam);
    b.setViewportSize(512.0f, 512.0f);

    vne::interaction::CameraCommandPayload p;
    p.x_px = 256.0f;
    p.y_px = 256.0f;
    p.zoom_factor = 1.2f;

    std::ignore = b.onAction(vne::interaction::CameraActionType::eZoomAtCursor, p, 0.016);
    b.onUpdate(0.016);

    EXPECT_GT(b.getWorldUnitsPerPixel(), 0.0f);
}

}  // namespace vne_interaction_test
