/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator_factory.h"

#include "vertexnova/interaction/interaction_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

class CameraManipulatorFactoryTest : public testing::Test {
   protected:
    vne::interaction::CameraManipulatorFactory factory_;
};

TEST_F(CameraManipulatorFactoryTest, CreateOrbitReturnsNonNull) {
    auto manip = factory_.create(vne::interaction::CameraManipulatorType::eOrbit);
    ASSERT_NE(manip, nullptr);
}

TEST_F(CameraManipulatorFactoryTest, CreateArcballReturnsNonNull) {
    auto manip = factory_.create(vne::interaction::CameraManipulatorType::eArcball);
    ASSERT_NE(manip, nullptr);
}

TEST_F(CameraManipulatorFactoryTest, CreateFpsReturnsNonNull) {
    auto manip = factory_.create(vne::interaction::CameraManipulatorType::eFps);
    ASSERT_NE(manip, nullptr);
}

TEST_F(CameraManipulatorFactoryTest, CreateFlyReturnsNonNull) {
    auto manip = factory_.create(vne::interaction::CameraManipulatorType::eFly);
    ASSERT_NE(manip, nullptr);
}

TEST_F(CameraManipulatorFactoryTest, CreateOrthoPanZoomReturnsNonNull) {
    auto manip = factory_.create(vne::interaction::CameraManipulatorType::eOrthoPanZoom);
    ASSERT_NE(manip, nullptr);
}

TEST_F(CameraManipulatorFactoryTest, CreateFollowReturnsNonNull) {
    auto manip = factory_.create(vne::interaction::CameraManipulatorType::eFollow);
    ASSERT_NE(manip, nullptr);
}

TEST_F(CameraManipulatorFactoryTest, AllTypesReturnUniqueInstances) {
    using Type = vne::interaction::CameraManipulatorType;
    const Type types[] = {Type::eOrbit, Type::eArcball, Type::eFps, Type::eFly, Type::eOrthoPanZoom, Type::eFollow};

    for (const Type type : types) {
        auto manip_a = factory_.create(type);
        auto manip_b = factory_.create(type);
        ASSERT_NE(manip_a, nullptr);
        ASSERT_NE(manip_b, nullptr);
        EXPECT_NE(manip_a.get(), manip_b.get()) << "Expected unique instances for type " << static_cast<int>(type);
    }
}

TEST_F(CameraManipulatorFactoryTest, CreatedManipulatorsAcceptViewportSize) {
    using Type = vne::interaction::CameraManipulatorType;
    const Type types[] = {Type::eOrbit, Type::eArcball, Type::eFps, Type::eFly, Type::eOrthoPanZoom, Type::eFollow};

    for (const Type type : types) {
        auto manip = factory_.create(type);
        ASSERT_NE(manip, nullptr);
        EXPECT_NO_FATAL_FAILURE(manip->setViewportSize(1920.0f, 1080.0f));
    }
}

TEST_F(CameraManipulatorFactoryTest, CreatedManipulatorsHaveNonNegativeSceneScale) {
    using Type = vne::interaction::CameraManipulatorType;
    const Type types[] = {Type::eOrbit, Type::eArcball, Type::eFps, Type::eFly, Type::eOrthoPanZoom, Type::eFollow};

    for (const Type type : types) {
        auto manip = factory_.create(type);
        ASSERT_NE(manip, nullptr);
        EXPECT_GE(manip->getSceneScale(), 0.0f) << "Negative scene scale for type " << static_cast<int>(type);
    }
}

TEST_F(CameraManipulatorFactoryTest, MultipleFactoryInstancesProduceIndependentManipulators) {
    vne::interaction::CameraManipulatorFactory other_factory;
    auto manip_a = factory_.create(vne::interaction::CameraManipulatorType::eOrbit);
    auto manip_b = other_factory.create(vne::interaction::CameraManipulatorType::eOrbit);
    ASSERT_NE(manip_a, nullptr);
    ASSERT_NE(manip_b, nullptr);
    EXPECT_NE(manip_a.get(), manip_b.get());
}

}  // namespace vne_interaction_test
