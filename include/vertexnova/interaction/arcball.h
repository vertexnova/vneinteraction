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
 * @file arcball.h
 * @brief Screen-space arcball: maps cursor positions to a unit sphere and derives rotations.
 *
 * @par ProjectionMode::eHyperbolic (default)
 * Isotropic mapping using `min(viewport width, height)`, inner spherical cap
 * `z = √(R² − d²)` when `d < R/√2`, hyperbolic continuation `z = t²/d` with `t = R/√2` outside.
 * Screen Y uses `(y_px − center_y)` (positive downward). Common in production 3D viewports;
 * see e.g. `calctrackballvec` in public engine/editor sources.
 *
 * @par ProjectionMode::eRim
 * Hemisphere `z = √(1 − x² − y²)` inside the unit disk; outside, points map to the
 * equatorial rim (`z = 0`) in normalized coordinates. Classic textbook arcball variant.
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

namespace vne::interaction {

/**
 * @brief Arcball / virtual trackball for quaternion orbit rotation.
 *
 * Call @ref setViewport when the drawable size changes. For a drag, call @ref beginDrag at
 * pointer down, then each move: @ref cumulativeDeltaQuaternion for the rotation from drag start
 * to the current point, @ref previousOnSphere and @ref project for frame-to-frame inertia, and
 * @ref endFrame with the current pointer position.
 */
class VNE_INTERACTION_API Arcball {
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
    Arcball() noexcept = default;

    /**
     * @brief Update pixel size (e.g. from @ref CameraBehaviorBase::onResize).
     * @param size_px: (width, height) in pixels.
     */
    void setViewport(const vne::math::Vec2f& size_px) noexcept;

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
     * @brief Map screen coordinates to a **unit** vector on the sphere (camera / arcball space).
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
     * @brief Shortest-arc rotation quaternion from unit vectors @a from to @a to.
     * @param from: The starting unit vector.
     * @param to: The ending unit vector.
     * @return The quaternion.
     */
    [[nodiscard]] static vne::math::Quatf rotationBetween(const vne::math::Vec3f& from,
                                                          const vne::math::Vec3f& to) noexcept;

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
    ProjectionMode projection_mode_ = ProjectionMode::eHyperbolic;  //!< Projection mode.
    vne::math::Vec3f drag_start_on_sphere_{0.0f, 0.0f, 1.0f};       //!< Drag start on sphere.
    vne::math::Vec2f last_cursor_px_{};  //!< Previous pointer position for frame-to-frame inertia (x, y pixels).
};

}  // namespace vne::interaction
