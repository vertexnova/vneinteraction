/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

namespace vne_interaction_test {

TEST(VneInteraction, ManipulatorCreation) {
    auto factory = std::make_shared<vne::interaction::CameraManipulatorFactory>();
    ASSERT_NE(factory, nullptr);

    using Type = vne::interaction::CameraManipulatorType;
    const Type types[] = {Type::eOrbit, Type::eArcball, Type::eFps, Type::eFly, Type::eOrthoPanZoom, Type::eFollow};

    for (Type type : types) {
        auto manip = factory->create(type);
        ASSERT_NE(manip, nullptr) << "Failed to create manipulator for type " << static_cast<int>(type);
        manip->setViewportSize(1280.0f, 720.0f);
        float scale = manip->getSceneScale();
        EXPECT_GE(scale, 0.0f);
    }
}

TEST(VneInteraction, CameraAndOrbitManipulator) {
    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    ASSERT_NE(camera, nullptr);
    camera->setViewport(1280.0f, 720.0f);

    auto factory = std::make_shared<vne::interaction::CameraManipulatorFactory>();
    auto orbit = factory->create(vne::interaction::CameraManipulatorType::eOrbit);
    ASSERT_NE(orbit, nullptr);

    orbit->setCamera(camera);
    orbit->setViewportSize(1280.0f, 720.0f);
    orbit->resetState();

    vne::math::Vec3f min_world(-1.0f, -1.0f, -1.0f);
    vne::math::Vec3f max_world(1.0f, 1.0f, 1.0f);
    orbit->fitToAABB(min_world, max_world);

    float scale = orbit->getSceneScale();
    EXPECT_GE(scale, 0.0f);
    EXPECT_TRUE(orbit->supportsPerspective());
}

}  // namespace vne_interaction_test
