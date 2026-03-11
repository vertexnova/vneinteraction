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

    /** Check if this manipulator supports perspective projection. */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    /** Check if this manipulator supports orthographic projection. */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /** Set the camera to manipulate. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Reset manipulator state and animation. */
    void resetState() noexcept override;
    /** Adjust camera to frame the given bounding box. */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    /** Get world units per pixel for screen-to-world conversions. */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /** Set the world-space up vector (default: +Y). */
    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;
    /** Set the zoom interaction method. */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    /** Get the current zoom method. */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    /** Set the camera view direction preset (front, back, top, isometric, etc.). */
    void setViewDirection(ViewDirection dir) noexcept;
    /** Set the center of interest (pivot point) in world or camera space. */
    void setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept;
    /** Get the center of interest in world space. */
    [[nodiscard]] vne::math::Vec3f getCenterOfInterestWorld() const noexcept { return coi_world_; }
    /** Get the distance from camera to center of interest. */
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }
    /** Set the distance from camera to center of interest. */
    void setOrbitDistance(float distance) noexcept;
    /** Set the rotation speed multiplier (default: 1.0). */
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = std::max(0.0f, speed); }
    /** Get the rotation speed multiplier. */
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }
    /** Set the pan speed multiplier (default: 1.0). */
    void setPanSpeed(float speed) noexcept { pan_speed_ = std::max(0.0f, speed); }
    /** Get the pan speed multiplier. */
    [[nodiscard]] float getPanSpeed() const noexcept { return pan_speed_; }
    /** Set the zoom speed for scroll/pinch zoom (default: 1.5). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    /** Set the FOV zoom speed for scroll/pinch (perspective only, default: 1.5). */
    void setFovZoomSpeed(float speed) noexcept { fov_zoom_speed_ = std::max(0.01f, speed); }
    /** Get the FOV zoom speed. */
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }
    /** Set the rotation damping factor for inertia (default: 0.0). */
    void setRotationDamping(float damping) noexcept { rot_damping_ = std::max(0.0f, damping); }
    /** Get the rotation damping factor. */
    [[nodiscard]] float getRotationDamping() const noexcept { return rot_damping_; }
    /** Set the pan damping factor for momentum panning (default: 0.0). */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    /** Get the pan damping factor. */
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }
    /** Set the mouse button mapping for rotate/pan actions. */
    void setButtonMap(const ButtonMap& map) noexcept { button_map_ = map; }
    /** Get the current mouse button mapping. */
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
