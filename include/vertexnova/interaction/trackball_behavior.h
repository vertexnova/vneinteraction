#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file trackball_behavior.h
 * @brief Screen-space virtual trackball (arcball): maps cursor positions to a unit sphere and derives rotations.
 *
 * @par ProjectionMode::eHyperbolic (default)
 * Isotropic mapping using `min(viewport width, height)`, inner spherical cap
 * `z = √(R² − d²)` when `d < R/√2`, hyperbolic continuation `z = t²/d` with `t = R/√2` outside.
 * Pointer positions are mapped through @c mouseWindowToNDC (see @ref setGraphicsApi) so horizontal and
 * vertical drags match the same clip/NDC convention as orbit pan and zoom-to-cursor across graphics APIs.
 *
 * @par ProjectionMode::eRim
 * Hemisphere `z = √(1 − x² − y²)` inside the unit disk; outside, points map to the
 * equatorial rim (`z = 0`) in normalized coordinates. Classic textbook trackball variant.
 *
 * @par Symmetry with @ref OrbitBehavior
 * @ref OrbitBehavior owns yaw/pitch (degrees), pitch limits, and Euler drag inertia in one type.
 * @ref TrackballBehavior owns **sphere mapping** and **ball-space** frame deltas (@ref BallFrameDelta via
 * @ref TrackballBehavior::ballFrameDeltaFromSpheres). Integrating release inertia into the orbit
 * **orientation quaternion** (world-space axis, rad/s) stays in @c OrbitalCameraManipulator because
 * it requires the current camera/orientation basis — same split as mapping ball axes to world.
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

namespace vne::interaction {

/**
 * @brief One frame of motion on the unit sphere (trackball / camera space), for inertia.
 *
 * @a axis_ball is in the same space as @ref project (right, screen-down, toward eye). The orbital
 * manipulator maps it to a world rotation axis using the orbit orientation basis.
 */
struct BallFrameDelta {
    bool valid = false;            //!< @c true if @a axis_ball and @a angle_rad are usable.
    vne::math::Vec3f axis_ball{};  //!< Unit rotation axis (ball space), if @a valid.
    float angle_rad = 0.0f;        //!< Shortest rotation angle from previous to current sample.
};

/**
 * @brief Virtual trackball for quaternion orbit rotation (rotation only; pan lives on @c OrbitalCameraManipulator).
 *
 * Call @ref setViewport when the drawable size changes. For a drag, call @ref beginDrag at
 * pointer down, then each move: @ref cumulativeDeltaQuaternion for the rotation from drag start
 * to the current point, @ref previousOnSphere and @ref project for frame-to-frame inertia, and
 * @ref endFrame with the current pointer position.
 *
 * For release inertia, use @ref ballFrameDeltaFromSpheres with consecutive @ref project samples.
 */
class VNE_INTERACTION_API TrackballBehavior {
   public:
    /**
     * @brief Screen-to-sphere mapping. Default: @ref eHyperbolic.
     * @enum eHyperbolic: Spherical cap, then hyperbolic continuation outside the cap.
     * @enum eRim: Hemisphere inside the unit disk; equatorial rim (z = 0) beyond it.
     */
    enum class ProjectionMode {
        eHyperbolic = 0,  //!< Spherical cap, then hyperbolic continuation outside the cap.
        eRim = 1          //!< Hemisphere inside the unit disk; equatorial rim (z = 0) beyond it.
    };

   public:
    TrackballBehavior() noexcept = default;

    /**
     * @brief Update pixel size (e.g. from @ref CameraManipulatorBase::onResize).
     * @param size_px: (width, height) in pixels.
     */
    void setViewport(const vne::math::Vec2f& size_px) noexcept;

    /**
     * @brief Graphics API for window → NDC mapping in @ref project (default OpenGL).
     * Must match @c ICamera::getGraphicsApi() / @ref CameraManipulatorBase::graphicsApi().
     */
    void setGraphicsApi(vne::math::GraphicsApi api) noexcept { graphics_api_ = api; }
    [[nodiscard]] vne::math::GraphicsApi getGraphicsApi() const noexcept { return graphics_api_; }

    /**
     * @brief Set the projection mode.
     * @param mode: The projection mode.
     */
    void setProjectionMode(ProjectionMode mode) noexcept { projection_mode_ = mode; }

    /**
     * @brief Get the projection mode.
     * @return The projection mode.
     */
    [[nodiscard]] ProjectionMode getProjectionMode() const noexcept { return projection_mode_; }

    /**
     * @brief Map screen coordinates to a **unit** vector on the sphere (camera / trackball space).
     * @param cursor_px: (x, y) in pixels.
     * @return The projected point on the sphere.
     */
    [[nodiscard]] vne::math::Vec3f project(const vne::math::Vec2f& cursor_px) const noexcept;

    /**
     * @brief Start a drag: records the sphere point and initial cursor for frame-to-frame inertia.
     * @param cursor_px: (x, y) in pixels.
     */
    void beginDrag(const vne::math::Vec2f& cursor_px) noexcept;

    /**
     * @brief Quaternion rotating from the drag-start sphere point to @a project(cursor_px).
     * @param cursor_px: (x, y) in pixels.
     * @return The quaternion.
     */
    [[nodiscard]] vne::math::Quatf cumulativeDeltaQuaternion(const vne::math::Vec2f& cursor_px) const noexcept;

    /**
     * @brief Shortest-arc rotation quaternion taking @a from to @a to (same construction as @c Quatf::fromToRotation).
     * @param from: The starting direction (normalized if non-zero).
     * @param to: The target direction (normalized if non-zero).
     * @return The quaternion.
     */
    [[nodiscard]] static vne::math::Quatf rotationBetween(const vne::math::Vec3f& from,
                                                          const vne::math::Vec3f& to) noexcept;

    /**
     * @brief Instantaneous rotation axis and angle between two consecutive unit sphere samples.
     * @param prev_sphere_unit Previous sample (e.g. @ref previousOnSphere before @ref project(cursor)).
     * @param curr_sphere_unit Current sample (e.g. @ref project(cursor)).
     * @return Ball-space axis and angle; @c valid is false for a degenerate (near-zero) move.
     */
    [[nodiscard]] static BallFrameDelta ballFrameDeltaFromSpheres(const vne::math::Vec3f& prev_sphere_unit,
                                                                  const vne::math::Vec3f& curr_sphere_unit) noexcept;

    /**
     * @brief Sphere points for inertia: previous frame cursor vs current (camera space).
     * @return The sphere point.
     */
    [[nodiscard]] vne::math::Vec3f previousOnSphere() const noexcept;

    /**
     * @brief Call at end of each drag step so the next step's @ref previousOnSphere matches this frame.
     * @param cursor_px: (x, y) in pixels.
     */
    void endFrame(const vne::math::Vec2f& cursor_px) noexcept;

    /**
     * @brief Clear drag bookkeeping (e.g. on reset).
     */
    void reset() noexcept;

   private:
    /**
     * @brief Project the point on the hyperbolic surface.
     * @param rx: The x coordinate.
     * @param ry: The y coordinate.
     * @return The projected point on the hyperbolic surface.
     */
    [[nodiscard]] vne::math::Vec3f projectHyperbolic(float rx, float ry) const noexcept;

    /**
     * @brief Project the point on the rim surface.
     * @param rx: The x coordinate.
     * @param ry: The y coordinate.
     * @return The projected point on the rim surface.
     */
    [[nodiscard]] vne::math::Vec3f projectRim(float rx, float ry) const noexcept;

   private:
    vne::math::Vec2f viewport_px_{};  //!< Viewport size in pixels (width = x, height = y).
    vne::math::GraphicsApi graphics_api_{vne::math::GraphicsApi::eOpenGL};
    ProjectionMode projection_mode_ = ProjectionMode::eHyperbolic;  //!< Projection mode.
    vne::math::Vec3f drag_start_on_sphere_{0.0f, 0.0f, 1.0f};       //!< Drag start on sphere.
    vne::math::Vec2f last_cursor_px_{};  //!< Previous pointer position for frame-to-frame inertia (x, y pixels).
};

}  // namespace vne::interaction
