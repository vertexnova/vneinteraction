#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/detail/orbit_style_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

/**
 * @brief Orbit-style camera manipulator with rotation around a center of interest.
 *
 * Provides classic 3D modeler-style orbit camera control. The camera orbits around a pivot point
 * (center of interest) in world space. Supports multiple interaction modes including:
 * - Rotation via mouse drag
 * - Panning via middle/right mouse button
 * - Zoom via scroll wheel or viewport scaling
 *
 * Supports both perspective and orthographic cameras. Multiple zoom methods available:
 * - eDollyToCoi: Move camera along view direction
 * - eSceneScale: Scale the entire scene relative to camera
 * - eChangeFov: Adjust field of view (perspective only)
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, OrbitStyleBase
 */
class VNE_INTERACTION_API OrbitManipulator final : public OrbitStyleBase {
   public:
    /** Construct orbit manipulator with default settings. */
    OrbitManipulator() noexcept;
    /** Destroy orbit manipulator. */
    ~OrbitManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    OrbitManipulator(const OrbitManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    OrbitManipulator& operator=(const OrbitManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    OrbitManipulator(OrbitManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    OrbitManipulator& operator=(OrbitManipulator&&) noexcept = default;

    /**
     * @brief Check if this manipulator supports perspective projection.
     * @return true (orbit supports perspective)
     */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }

    /**
     * @brief Check if this manipulator supports orthographic projection.
     * @return true (orbit supports orthographic)
     */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /**
     * @brief Set the camera to manipulate.
     * @param camera Shared pointer to the camera (may be nullptr to detach)
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Reset manipulator state and animation. */
    void resetState() noexcept override;

    /**
     * @brief Adjust camera to frame the given bounding box.
     * @param min_world Minimum corner of the AABB in world space
     * @param max_world Maximum corner of the AABB in world space
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;

    /**
     * @brief Get world units per pixel for screen-to-world conversions.
     * @return World units per pixel at the center of the viewport
     */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /**
     * @brief Set the world-space up vector (default: +Y).
     * @param world_up Normalized up vector; ignored if zero length
     */
    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;

    /**
     * @brief Set the zoom interaction method.
     * @param method Zoom method (eDollyToCoi, eSceneScale, eChangeFov)
     */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }

    /**
     * @brief Get the current zoom method.
     * @return Current zoom method
     */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /**
     * @brief Set the camera view direction preset (front, back, top, isometric, etc.).
     * @param dir View direction preset
     */
    void setViewDirection(ViewDirection dir) noexcept;

    /**
     * @brief Set the center of interest (pivot point) in world or camera space.
     * @param coi Center of interest position
     * @param space eWorldSpace for absolute coords, eCameraSpace for camera-relative
     */
    void setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept;

    /**
     * @brief Get the center of interest in world space.
     * @return COI position in world coordinates
     */
    [[nodiscard]] vne::math::Vec3f getCenterOfInterestWorld() const noexcept { return coi_world_; }

    /**
     * @brief Get the distance from camera to center of interest.
     * @return Orbit distance in world units
     */
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }

    /**
     * @brief Set the distance from camera to center of interest.
     * @param distance Distance in world units (clamped to minimum)
     */
    void setOrbitDistance(float distance) noexcept;
    /**
     * @brief Set the rotation speed multiplier (default: 1.0).
     * @param speed Speed multiplier (clamped to >= 0)
     */
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = std::max(0.0f, speed); }

    /**
     * @brief Get the rotation speed multiplier.
     * @return Current rotation speed
     */
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }

    /**
     * @brief Set the pan speed multiplier (default: 1.0).
     * @param speed Speed multiplier (clamped to >= 0)
     */
    void setPanSpeed(float speed) noexcept { pan_speed_ = std::max(0.0f, speed); }

    /**
     * @brief Get the pan speed multiplier.
     * @return Current pan speed
     */
    [[nodiscard]] float getPanSpeed() const noexcept { return pan_speed_; }

    /**
     * @brief Set the zoom speed for scroll/pinch zoom (default: 1.5).
     * @param speed Zoom speed (> 1.0, clamped to >= 0.01)
     */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }

    /**
     * @brief Set the FOV zoom speed for scroll/pinch (perspective only, default: 1.5).
     * @param speed FOV zoom speed (clamped to >= 0.01)
     */
    void setFovZoomSpeed(float speed) noexcept { fov_zoom_speed_ = std::max(0.01f, speed); }

    /**
     * @brief Get the FOV zoom speed.
     * @return Current FOV zoom speed
     */
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }

    /**
     * @brief Set the rotation damping factor for inertia (default: 0.0).
     * @param damping Damping factor (clamped to >= 0)
     */
    void setRotationDamping(float damping) noexcept { rot_damping_ = std::max(0.0f, damping); }

    /**
     * @brief Get the rotation damping factor.
     * @return Current rotation damping
     */
    [[nodiscard]] float getRotationDamping() const noexcept { return rot_damping_; }

    /**
     * @brief Set the pan damping factor for momentum panning (default: 0.0).
     * @param damping Damping factor (clamped to >= 0)
     */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }

    /**
     * @brief Get the pan damping factor.
     * @return Current pan damping
     */
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /**
     * @brief Set the mouse button mapping for rotate/pan actions.
     * @param map Button map with rotate and pan button indices
     */
    void setButtonMap(const ButtonMap& map) noexcept { button_map_ = map; }

    /**
     * @brief Get the current mouse button mapping.
     * @return Reference to the current button map
     */
    [[nodiscard]] const ButtonMap& getButtonMap() const noexcept { return button_map_; }

   private:
    /** Compute forward direction vector from current yaw/pitch. */
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept override;
    /** Synchronize manipulator state from current camera properties. */
    void syncFromCamera() noexcept override;
    /** Apply manipulator state to update the camera. */
    void applyToCamera() noexcept override;
    /** Begin rotation from screen position. */
    void beginRotate(float x_px, float y_px) noexcept override;
    /** Handle rotation drag with delta movement. */
    void dragRotate(float delta_x_px, float delta_y_px, double delta_time) noexcept override;
    /** End rotation and set up inertia if damping is enabled. */
    void endRotate(double delta_time) noexcept override;
    /** Apply rotational inertia decay. */
    void applyInertia(double delta_time) noexcept override;

    float yaw_deg_ = 0.0f;              //!< Rotation yaw angle in degrees (left-right)
    float pitch_deg_ = 0.0f;            //!< Rotation pitch angle in degrees (up-down)
    float inertia_rot_speed_x_ = 0.0f;  //!< Inertia velocity for yaw in degrees per second
    float inertia_rot_speed_y_ = 0.0f;  //!< Inertia velocity for pitch in degrees per second
};

}  // namespace vne::interaction
