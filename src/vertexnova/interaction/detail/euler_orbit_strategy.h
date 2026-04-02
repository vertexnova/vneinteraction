#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file euler_orbit_strategy.h
 * @brief Euler (yaw/pitch) rotation strategy for OrbitalCameraManipulator.
 *
 * Wraps OrbitBehavior and owns all Euler-specific state and inertia.
 * Internal header — not installed, not part of the public API.
 */

#include "rotation_strategy.h"

#include "vertexnova/interaction/orbit_behavior.h"

namespace vne::interaction {

/**
 * @brief Classic yaw/pitch orbit strategy.
 *
 * Delegates angle storage, inertia, and front-direction computation to OrbitBehavior.
 */
class EulerOrbitStrategy final : public IRotationStrategy {
   public:
    EulerOrbitStrategy() noexcept = default;

    void beginRotate(float /*x_px*/, float /*y_px*/, OrbitalContext& /*ctx*/) noexcept override { orbit_.beginDrag(); }

    void dragRotate(float /*x_px*/,
                    float /*y_px*/,
                    float delta_x_px,
                    float delta_y_px,
                    double delta_time,
                    OrbitalContext& /*ctx*/) noexcept override {
        orbit_.applyDrag(delta_x_px, delta_y_px, rotation_speed_, delta_time, kMinDtForInertia);
    }

    void endRotate(bool inertia_enabled, double /*dt*/) noexcept override {
        if (!inertia_enabled) {
            orbit_.clearInertia();
        }
    }

    bool stepInertia(float dt, float damping, OrbitalContext& /*ctx*/) noexcept override {
        constexpr float kInertiaThreshold = 1e-3f;
        return orbit_.stepInertia(dt, damping, kInertiaThreshold);
    }

    void syncFromCamera(OrbitalContext& ctx) noexcept override {
        if (!ctx.camera) {
            return;
        }
        vne::math::Vec3f front = ctx.coi_world - ctx.camera->getPosition();
        const float dist = front.length();
        if (dist < 1e-6f) {
            return;
        }
        front /= dist;
        orbit_.syncFromViewDirection(ctx.world_up, front);
    }

    [[nodiscard]] vne::math::Vec3f computeBackDirection(OrbitalContext& ctx) const noexcept override {
        // Euler back = -front
        return -orbit_.computeFrontDirection(ctx.world_up);
    }

    [[nodiscard]] vne::math::Vec3f computeUpHint(OrbitalContext& ctx) const noexcept override {
        const vne::math::Vec3f front = orbit_.computeFrontDirection(ctx.world_up);
        // right = front × world_up; up = right × front
        vne::math::Vec3f r = front.cross(ctx.world_up);
        const float rlen = r.length();
        if (rlen < 1e-6f) {
            return ctx.world_up;
        }
        r /= rlen;
        const vne::math::Vec3f up = r.cross(front);
        const float ulen = up.length();
        return (ulen < 1e-6f) ? ctx.world_up : (up / ulen);
    }

    void applyViewDirection(float yaw_deg, float pitch_deg, OrbitalContext& /*ctx*/) noexcept override {
        orbit_.setYawPitch(yaw_deg, pitch_deg);
    }

    void setRotationSpeed(float speed) noexcept override { rotation_speed_ = speed; }
    void clearInertia() noexcept override { orbit_.clearInertia(); }

    void reset(OrbitalContext& /*ctx*/) noexcept override { orbit_.clearInertia(); }

    // ---- Euler-only accessors for OrbitalCameraManipulator ----------------------

    [[nodiscard]] OrbitBehavior& orbitBehavior() noexcept { return orbit_; }
    [[nodiscard]] const OrbitBehavior& orbitBehavior() const noexcept { return orbit_; }

   private:
    OrbitBehavior orbit_;
    float rotation_speed_ = 0.2f;
    static constexpr double kMinDtForInertia = 0.001;
};

}  // namespace vne::interaction
