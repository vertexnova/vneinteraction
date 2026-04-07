/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <cmath>

namespace vne_interaction_test {

namespace {

const vne::math::Vec3f kWorldUp(0.0f, 1.0f, 0.0f);

}  // namespace

// TEST(OrbitBehavior, DefaultYawPitchZero) {
//     vne::interaction::OrbitBehavior o;
//     EXPECT_FLOAT_EQ(o.getYawDeg(), 0.0f);
//     EXPECT_FLOAT_EQ(o.getPitchDeg(), 0.0f);
// }

// TEST(OrbitBehavior, SetYawPitchClampsPitch) {
//     vne::interaction::OrbitBehavior o;
//     o.setYawPitch(10.0f, 100.0f);
//     EXPECT_FLOAT_EQ(o.getYawDeg(), 10.0f);
//     EXPECT_FLOAT_EQ(o.getPitchDeg(), vne::interaction::OrbitBehavior::kDefaultPitchMaxDeg);
//     o.setYawPitch(0.0f, -100.0f);
//     EXPECT_FLOAT_EQ(o.getPitchDeg(), vne::interaction::OrbitBehavior::kDefaultPitchMinDeg);
// }

// TEST(OrbitBehavior, SetPitchLimits) {
//     vne::interaction::OrbitBehavior o;
//     o.setPitchLimits(-45.0f, 45.0f);
//     o.setYawPitch(0.0f, 60.0f);
//     EXPECT_FLOAT_EQ(o.getPitchDeg(), 45.0f);
// }

// TEST(OrbitBehavior, ApplyDragClampsPitch) {
//     vne::interaction::OrbitBehavior o;
//     // Negative dy = mouse up → pitch increases; large magnitude clamps to max pitch.
//     o.applyDrag(0.0f, -10000.0f, 1.0f, 0.016, 0.001);
//     EXPECT_FLOAT_EQ(o.getPitchDeg(), vne::interaction::OrbitBehavior::kDefaultPitchMaxDeg);
// }

// TEST(OrbitBehavior, ApplyDrag_MouseUpIncreasesPitch) {
//     vne::interaction::OrbitBehavior o;
//     o.applyDrag(0.0f, -20.0f, 0.5f, 0.016, 0.001);
//     EXPECT_GT(o.getPitchDeg(), 0.0f);
//     o.setYawPitch(0.0f, 0.0f);
//     o.applyDrag(0.0f, 20.0f, 0.5f, 0.016, 0.001);
//     EXPECT_LT(o.getPitchDeg(), 0.0f);
// }

// TEST(OrbitBehavior, SyncFromViewDirectionRoundTrip) {
//     vne::interaction::OrbitBehavior o;
//     o.setYawPitch(30.0f, -20.0f);
//     const vne::math::Vec3f dir = o.computeFrontDirection(kWorldUp);
//     EXPECT_NEAR(dir.length(), 1.0f, 1e-5f);

//     vne::interaction::OrbitBehavior o2;
//     o2.syncFromViewDirection(kWorldUp, dir);
//     EXPECT_NEAR(o2.getYawDeg(), 30.0f, 0.05f);
//     EXPECT_NEAR(o2.getPitchDeg(), -20.0f, 0.05f);
// }

// TEST(OrbitBehavior, StepInertiaDamps) {
//     vne::interaction::OrbitBehavior o;
//     o.applyDrag(100.0f, 0.0f, 0.5f, 0.016, 0.001);
//     const float yaw_before = o.getYawDeg();
//     const float speed0 = std::abs(o.getInertiaSpeedXDegPerSec());
//     EXPECT_GT(speed0, 1.0f);
//     for (int i = 0; i < 240; ++i) {
//         o.stepInertia(1.0f / 60.0f, 8.0f, 1e-3f);
//     }
//     EXPECT_LT(std::abs(o.getInertiaSpeedXDegPerSec()), speed0 * 0.01f);
//     EXPECT_GT(o.getYawDeg(), yaw_before);
// }

// TEST(OrbitBehavior, BeginDragClearsInertia) {
//     vne::interaction::OrbitBehavior o;
//     o.applyDrag(50.0f, 0.0f, 1.0f, 0.016, 0.001);
//     EXPECT_GT(std::abs(o.getInertiaSpeedXDegPerSec()), 0.0f);
//     o.beginDrag();
//     EXPECT_FLOAT_EQ(o.getInertiaSpeedXDegPerSec(), 0.0f);
//     EXPECT_FLOAT_EQ(o.getInertiaSpeedYDegPerSec(), 0.0f);
// }

// TEST(OrbitBehavior, ApplyDragZeroDeltaTimeDoesNotPoisonInertia) {
//     vne::interaction::OrbitBehavior o;
//     o.beginDrag();
//     o.applyDrag(100.0f, 0.0f, 0.5f, 0.0, 0.001);
//     EXPECT_TRUE(std::isfinite(o.getInertiaSpeedXDegPerSec()));
//     EXPECT_TRUE(std::isfinite(o.getInertiaSpeedYDegPerSec()));
//     EXPECT_FLOAT_EQ(o.getInertiaSpeedXDegPerSec(), 0.0f);
// }

// TEST(OrbitBehavior, ApplyDragNonPositiveMinDeltaTimeClampedForInertia) {
//     vne::interaction::OrbitBehavior o;
//     o.beginDrag();
//     o.applyDrag(100.0f, 0.0f, 0.5f, 0.016, 0.0);
//     const float speed = std::abs(o.getInertiaSpeedXDegPerSec());
//     EXPECT_TRUE(std::isfinite(speed));
//     EXPECT_GT(speed, 1.0f);
// }

}  // namespace vne_interaction_test
