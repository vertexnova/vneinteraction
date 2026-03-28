/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/arcball.h"

#include <vertexnova/math/core/core.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace vne_interaction_test {

namespace {

[[nodiscard]] float quatAngleRad(const vne::math::Quatf& q) noexcept {
    return 2.0f * std::acos(std::clamp(q.w, -1.0f, 1.0f));
}

void expectUnitQuat(const vne::math::Quatf& q) {
    EXPECT_NEAR(q.length(), 1.0f, 1e-4f);
}

void expectRotatesFromTo(const vne::math::Quatf& q,
                         const vne::math::Vec3f& from_unit,
                         const vne::math::Vec3f& to_unit,
                         float cos_tol) {
    expectUnitQuat(q);
    const vne::math::Vec3f rotated = (q * from_unit).normalized();
    EXPECT_GE(rotated.dot(to_unit), cos_tol) << "q*from should align with to";
}

}  // namespace

// ---------------------------------------------------------------------------
// Projection (hyperbolic / rim)
// ---------------------------------------------------------------------------

TEST(Arcball, HyperbolicCenterIsUnitAndFrontHemisphere) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    ASSERT_EQ(a.getProjectionMode(), vne::interaction::Arcball::ProjectionMode::eHyperbolic);
    const auto v = a.project(vne::math::Vec2f(400.0f, 300.0f));
    EXPECT_NEAR(v.length(), 1.0f, 1e-5f);
    EXPECT_GT(v.z(), 0.0f);
    EXPECT_NEAR(v.x(), 0.0f, 1e-6f);
    EXPECT_NEAR(v.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(v.z(), 1.0f, 1e-5f);
}

TEST(Arcball, RimCenterMatchesHyperbolicAtOrigin) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    a.setProjectionMode(vne::interaction::Arcball::ProjectionMode::eRim);
    const auto v = a.project(vne::math::Vec2f(400.0f, 300.0f));
    EXPECT_NEAR(v.length(), 1.0f, 1e-5f);
    EXPECT_GT(v.z(), 0.0f);
}

// ---------------------------------------------------------------------------
// rotationBetween (static helper used by cumulative delta)
// ---------------------------------------------------------------------------

TEST(Arcball, RotationBetweenIdentity) {
    const vne::math::Vec3f v(0.3f, -0.4f, 0.8f);
    const auto n = v.normalized();
    const auto q = vne::interaction::Arcball::rotationBetween(n, n);
    expectUnitQuat(q);
    EXPECT_NEAR(q.w, 1.0f, 1e-4f);
    EXPECT_NEAR(q.x, 0.0f, 1e-4f);
    EXPECT_NEAR(q.y, 0.0f, 1e-4f);
    EXPECT_NEAR(q.z, 0.0f, 1e-4f);
}

TEST(Arcball, RotationBetweenSmallAngle) {
    const vne::math::Vec3f from(0.0f, 0.0f, 1.0f);
    const vne::math::Vec3f to = (from + vne::math::Vec3f(0.02f, -0.01f, 0.0f)).normalized();
    const auto q = vne::interaction::Arcball::rotationBetween(from, to);
    expectRotatesFromTo(q, from, to, 0.99f);
    EXPECT_LT(quatAngleRad(q), 0.15f);
}

TEST(Arcball, RotationBetweenSwappedEndpointsAreInverseRotations) {
    // q(a,b) and q(b,a) compose to identity for proper shortest-arc rotations.
    const vne::math::Vec3f from = vne::math::Vec3f(0.3f, -0.4f, 0.7f).normalized();
    const vne::math::Vec3f to = vne::math::Vec3f(-0.2f, 0.5f, 0.75f).normalized();
    const auto q_ab = vne::interaction::Arcball::rotationBetween(from, to);
    const auto q_ba = vne::interaction::Arcball::rotationBetween(to, from);
    const vne::math::Quatf prod = (q_ab * q_ba).normalized();
    EXPECT_NEAR(prod.w, 1.0f, 1e-4f);
    EXPECT_NEAR(prod.x, 0.0f, 1e-4f);
    EXPECT_NEAR(prod.y, 0.0f, 1e-4f);
    EXPECT_NEAR(prod.z, 0.0f, 1e-4f);
}

TEST(Arcball, RotationBetweenOppositeVectorsIs180Degrees) {
    const vne::math::Vec3f from(0.0f, 0.0f, 1.0f);
    const vne::math::Vec3f to(0.0f, 0.0f, -1.0f);
    const auto q = vne::interaction::Arcball::rotationBetween(from, to);
    expectUnitQuat(q);
    const vne::math::Vec3f rotated = (q * from).normalized();
    EXPECT_GT(rotated.dot(to), 0.99f);
    EXPECT_NEAR(quatAngleRad(q), 3.14159265f, 0.02f);
}

TEST(Arcball, RotationBetweenMatchesFromToRotation) {
    const vne::math::Vec3f a = vne::math::Vec3f(0.2f, -0.3f, 0.9f).normalized();
    const vne::math::Vec3f b = vne::math::Vec3f(-0.1f, 0.55f, 0.72f).normalized();
    const auto q_arc = vne::interaction::Arcball::rotationBetween(a, b);
    const auto q_ref = vne::math::Quatf::fromToRotation(a, b);
    const float d = std::abs(vne::math::Quatf::dot(q_arc, q_ref));
    EXPECT_GT(d, 0.999f);
}

// ---------------------------------------------------------------------------
// cumulativeDeltaQuaternion + drag lifecycle
// ---------------------------------------------------------------------------

TEST(Arcball, CumulativeDeltaSmallDragSmallAngle) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    a.beginDrag(vne::math::Vec2f(400.0f, 300.0f));
    const vne::math::Vec3f start = a.project(vne::math::Vec2f(400.0f, 300.0f));
    const vne::math::Vec2f end_small(430.0f, 305.0f);
    const auto q = a.cumulativeDeltaQuaternion(end_small);
    expectUnitQuat(q);
    const vne::math::Vec3f end_sphere = a.project(end_small);
    expectRotatesFromTo(q, start, end_sphere, 0.98f);
    EXPECT_LT(quatAngleRad(q), 0.35f);
}

TEST(Arcball, CumulativeDeltaLargeDragLargerAngleThanSmall) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    a.beginDrag(vne::math::Vec2f(400.0f, 300.0f));

    const vne::math::Vec2f end_small(430.0f, 305.0f);
    const float ang_small = quatAngleRad(a.cumulativeDeltaQuaternion(end_small));

    a.beginDrag(vne::math::Vec2f(400.0f, 300.0f));
    const vne::math::Vec2f end_large(650.0f, 450.0f);
    const float ang_large = quatAngleRad(a.cumulativeDeltaQuaternion(end_large));

    EXPECT_GT(ang_large, ang_small);
}

TEST(Arcball, EndFrameUpdatesPreviousForInertiaPath) {
    vne::interaction::Arcball a;
    a.setViewport(vne::math::Vec2f(800.0f, 600.0f));
    a.beginDrag(vne::math::Vec2f(400.0f, 300.0f));
    const vne::math::Vec2f p1(410.0f, 300.0f);
    a.endFrame(p1);
    EXPECT_NEAR(a.previousOnSphere().dot(a.project(p1)), 1.0f, 1e-4f);
    const vne::math::Vec2f p2(450.0f, 320.0f);
    // previous is still p1 until endFrame(p2)
    const vne::math::Vec3f prev = a.previousOnSphere();
    a.endFrame(p2);
    EXPECT_NEAR(a.previousOnSphere().dot(a.project(p2)), 1.0f, 1e-4f);
    EXPECT_LT(std::abs(prev.dot(a.project(p2))), 0.999f);
}

}  // namespace vne_interaction_test
