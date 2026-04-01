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
 * @file orbit_behavior.h
 * @brief Classic yaw/pitch orbit around a world-up axis (Euler angles).
 *
 * Complements @ref TrackballBehavior: this type stores **degrees** on the ground track (yaw) and
 * elevation (pitch) relative to a horizontal plane derived from \c world_up (see implementation).
 * Screen motion maps to deltas with a caller-supplied **deg/pixel** scale (see @ref applyDrag).
 *
 * @par Reference frame
 * Given a normalized @a world_up, forward and right axes are built so yaw is rotation about
 * world-up and pitch tilts the view direction toward or away from that axis. The resulting
 * **view direction** is the unit vector from eye toward center of interest (COI), consistent
 * with @c OrbitalCameraManipulator::computeFront in Euler mode.
 *
 * @par Pitch limits
 * Pitch is clamped to @ref kDefaultPitchMinDeg .. @ref kDefaultPitchMaxDeg by default to avoid
 * the polar singularity; use @ref setPitchLimits for stricter CAD-style caps or slightly wider
 * ranges. @ref syncFromViewDirection does **not** clamp pitch (legacy camera sync); @ref setYawPitch,
 * @ref applyDrag, and @ref stepInertia clamp.
 *
 * @par Drag inertia
 * @ref applyDrag can record yaw/pitch rates (deg/s) for release coasting. @ref stepInertia integrates
 * one frame using the same exponential damping as @ref OrbitalCameraManipulator (via @c vne::math::damp).
 * Call @ref beginDrag or @ref clearInertia when a new rotation gesture starts or on reset.
 *
 * @par Symmetry with @ref TrackballBehavior
 * @ref OrbitBehavior centralizes **Euler** angles, limits, and **Euler** inertia. @ref TrackballBehavior centralizes
 * sphere mapping and **ball-space** frame deltas (@ref BallFrameDelta / @ref
 * TrackballBehavior::ballFrameDeltaFromSpheres).
 * **Trackball** orientation integration and world-axis mapping remain in @c OrbitalCameraManipulator because
 * they need the orbit quaternion — parallel to how Euler world view direction is assembled there from
 * @ref computeFrontDirection.
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

namespace vne::interaction {

/**
 * @brief Euler (yaw/pitch) orbit angles for inspect-style cameras.
 *
 * Typical usage with a behavior:
 * - On rotate begin: @ref beginDrag.
 * - Each move: @ref applyDrag with pointer deltas and @a rotation_speed_deg_per_px.
 * - While coasting: @ref stepInertia with frame @a delta_time_sec and behavior damping.
 * - When the camera is moved externally: @ref syncFromViewDirection to recover yaw/pitch.
 * - For framing presets: @ref setYawPitch or adjust limits with @ref setPitchLimits.
 */
class VNE_INTERACTION_API OrbitBehavior {
   public:
    /**
     * @brief Default lower pitch bound (degrees).
     * @details Keeps the look direction off the singularity at -90°; override with @ref setPitchLimits.
     */
    static constexpr float kDefaultPitchMinDeg = -89.0f;
    /**
     * @brief Default upper pitch bound (degrees).
     * @details Keeps the look direction off the singularity at +90°; override with @ref setPitchLimits.
     */
    static constexpr float kDefaultPitchMaxDeg = 89.0f;

    OrbitBehavior() noexcept = default;

    /**
     * @brief Current yaw about world-up (degrees).
     * @return Yaw in degrees.
     */
    [[nodiscard]] float getYawDeg() const noexcept { return yaw_deg_; }

    /**
     * @brief Current pitch elevation (degrees).
     * @return Pitch in degrees.
     */
    [[nodiscard]] float getPitchDeg() const noexcept { return pitch_deg_; }

    /**
     * @brief Set yaw and pitch; pitch is clamped to @ref getPitchMinDeg .. @ref getPitchMaxDeg.
     * @param yaw_deg   Yaw in degrees.
     * @param pitch_deg Pitch in degrees (clamped).
     */
    void setYawPitch(float yaw_deg, float pitch_deg) noexcept;

    /**
     * @brief Set inclusive pitch limits; current pitch is clamped into the new range.
     * @param min_deg Minimum pitch (degrees); must be less than @a max_deg.
     * @param max_deg Maximum pitch (degrees).
     */
    void setPitchLimits(float min_deg, float max_deg) noexcept;

    /**
     * @brief Current minimum pitch bound (degrees).
     * @return Lower pitch clamp.
     */
    [[nodiscard]] float getPitchMinDeg() const noexcept { return pitch_min_deg_; }

    /**
     * @brief Current maximum pitch bound (degrees).
     * @return Upper pitch clamp.
     */
    [[nodiscard]] float getPitchMaxDeg() const noexcept { return pitch_max_deg_; }

    /**
     * @brief Unit view direction (from eye toward COI) from current yaw/pitch.
     * @param world_up Normalized world up vector used to build the horizontal reference frame.
     * @return Normalized direction; if degenerate, falls back to the reference forward axis.
     */
    [[nodiscard]] vne::math::Vec3f computeFrontDirection(const vne::math::Vec3f& world_up) const noexcept;

    /**
     * @brief Recover yaw/pitch from a unit view direction (eye → COI).
     * @param world_up             Normalized world up.
     * @param view_direction_unit  Normalized direction from camera toward pivot/COI.
     * @note Pitch is **not** clamped here (matches legacy orbit sync from camera matrices).
     */
    void syncFromViewDirection(const vne::math::Vec3f& world_up, const vne::math::Vec3f& view_direction_unit) noexcept;

    /**
     * @brief Clear yaw/pitch inertia at pointer down (or before a new drag).
     */
    void beginDrag() noexcept;

    /**
     * @brief Apply a pointer delta: yaw += dx × speed, pitch −= dy × speed (pitch clamped).
     * @param delta_x_px                    Horizontal pointer delta (pixels).
     * @param delta_y_px                    Vertical pointer delta (pixels; top-left window space, Y down). Uses the
     *                                      same raw convention as typical OS/input APIs — not remapped through
     *                                      @c GraphicsApi NDC (Euler orbit is deg/pixel, not frustum samples).
     * @param rotation_speed_deg_per_px     Degrees of rotation per pixel (behavior tuning).
     * @param delta_time                    Frame time for this drag sample (seconds).
     * @param min_delta_time_for_inertia    Minimum dt to consider when updating inertia rates (seconds).
     *                                      Non-finite values log a warning and use an internal floor; non-positive
     *                                      values are clamped to that floor. Inertia rates are updated only when
     *                                      @a delta_time is finite, positive, and @c 1/delta_time is finite.
     */
    void applyDrag(float delta_x_px,
                   float delta_y_px,
                   float rotation_speed_deg_per_px,
                   double delta_time,
                   double min_delta_time_for_inertia) noexcept;

    /**
     * @brief Integrate inertia for one frame and damp angular rates.
     * @param delta_time_sec    Frame duration (seconds).
     * @param rot_damping       Damping factor (same convention as @ref OrbitalCameraManipulator::setRotationDamping).
     * @param inertia_threshold Minimum absolute rate (deg/s) to treat as moving.
     * @return @c true if yaw or pitch changed this frame.
     */
    bool stepInertia(float delta_time_sec, float rot_damping, float inertia_threshold) noexcept;

    /**
     * @brief Zero yaw/pitch inertia (e.g. on reset).
     */
    void clearInertia() noexcept;

    /**
     * @brief Yaw inertia rate (deg/s) for release coasting.
     */
    [[nodiscard]] float getInertiaSpeedXDegPerSec() const noexcept { return inertia_rot_speed_x_; }

    /**
     * @brief Pitch inertia rate (deg/s) for release coasting.
     */
    [[nodiscard]] float getInertiaSpeedYDegPerSec() const noexcept { return inertia_rot_speed_y_; }

   private:
    float yaw_deg_ = 0.0f;                       //!< Yaw about world-up (degrees).
    float pitch_deg_ = 0.0f;                     //!< Pitch elevation (degrees).
    float pitch_min_deg_ = kDefaultPitchMinDeg;  //!< Lower pitch clamp (degrees).
    float pitch_max_deg_ = kDefaultPitchMaxDeg;  //!< Upper pitch clamp (degrees).
    float inertia_rot_speed_x_ = 0.0f;           //!< Yaw rate for inertia (deg/s).
    float inertia_rot_speed_y_ = 0.0f;           //!< Pitch rate for inertia (deg/s).
};

}  // namespace vne::interaction
