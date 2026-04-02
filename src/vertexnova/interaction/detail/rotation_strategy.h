#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

/**
 * @file rotation_strategy.h
 * @brief Internal rotation strategy interface for OrbitalCameraManipulator.
 *
 * Each concrete strategy owns the rotation-specific state and implements all rotation
 * operations. Shared orbit state (COI, distance, world_up, camera, viewport) is passed
 * as an OrbitalContext reference so the strategy can read and modify shared pose.
 *
 * Internal header — not installed, not part of the public API.
 */

#include <vertexnova/math/core/core.h>
#include <vertexnova/math/viewport.h>
#include <vertexnova/scene/camera/camera.h>

#include <memory>

namespace vne::interaction {

/**
 * @brief Shared orbit state passed to rotation strategies.
 *
 * Strategies read these fields to drive camera updates and may write orientation
 * results back (e.g. syncing from camera on setViewDirection).
 */
struct OrbitalContext {
    std::shared_ptr<vne::scene::ICamera>& camera;  //!< Attached camera (may be null).
    vne::math::Vec3f& world_up;                    //!< World-space up vector.
    vne::math::Vec3f& coi_world;                   //!< Center of interest in world space.
    float& orbit_distance;                         //!< Eye–COI distance.
    const vne::math::Viewport& viewport;           //!< Current viewport (pixels).
    vne::math::GraphicsApi graphics_api;           //!< Active graphics API (NDC convention).
};

/**
 * @brief Abstract rotation strategy for OrbitalCameraManipulator.
 *
 * Implementations handle all rotation-mode-specific state and operations.
 * Pan, zoom, and inertia-pan remain in the manipulator.
 */
class IRotationStrategy {
   public:
    virtual ~IRotationStrategy() = default;

    // ---- drag gesture -----------------------------------------------------------

    /** Called at pointer down to initialize drag state. */
    virtual void beginRotate(float x_px, float y_px, OrbitalContext& ctx) noexcept = 0;

    /** Called each move event during a drag to update orientation and apply to camera. */
    virtual void dragRotate(float x_px,
                            float y_px,
                            float delta_x_px,
                            float delta_y_px,
                            double delta_time,
                            OrbitalContext& ctx) noexcept = 0;

    /** Called at pointer up; may capture inertia velocity. */
    virtual void endRotate(bool inertia_enabled, double delta_time) noexcept = 0;

    // ---- per-frame inertia ------------------------------------------------------

    /**
     * @brief Advance rotation inertia one frame.
     * @return true if orientation changed and camera should be updated.
     */
    virtual bool stepInertia(float dt, float damping, OrbitalContext& ctx) noexcept = 0;

    // ---- camera synchronization -------------------------------------------------

    /** Rebuild internal rotation state from the current camera pose. */
    virtual void syncFromCamera(OrbitalContext& ctx) noexcept = 0;

    /**
     * @brief Compute view direction for applying to camera.
     * @return Unit vector from COI toward eye (back direction, camera +Z in RH).
     */
    [[nodiscard]] virtual vne::math::Vec3f computeBackDirection(OrbitalContext& ctx) const noexcept = 0;

    /**
     * @brief Camera-space up hint (for lookAt).
     */
    [[nodiscard]] virtual vne::math::Vec3f computeUpHint(OrbitalContext& ctx) const noexcept = 0;

    // ---- view preset ------------------------------------------------------------

    /**
     * @brief Apply a view-direction preset.
     * Strategies that use angles (Euler) apply directly; trackball bakes the result into its quaternion.
     */
    virtual void applyViewDirection(float yaw_deg, float pitch_deg, OrbitalContext& ctx) noexcept = 0;

    // ---- settings ---------------------------------------------------------------

    virtual void setRotationSpeed(float speed) noexcept = 0;
    virtual void clearInertia() noexcept = 0;
    virtual void reset(OrbitalContext& ctx) noexcept = 0;
};

}  // namespace vne::interaction
