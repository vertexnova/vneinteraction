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
 * @brief Arcball-style camera manipulator with smooth quaternion-based rotation.
 *
 * Implements an arcball (virtual trackball) interaction model where screen coordinates
 * map to a 3D sphere. Rotating the mouse around this sphere creates an intuitive
 * 3-degree-of-freedom rotation.
 *
 * Features:
 * - Smooth interpolation using unit quaternions to avoid gimbal lock
 * - Persistent accumulated orientation prevents numerical drift via periodic normalization
 * - Support for inertia-based momentum scrolling
 * - Multiple zoom methods and resolution-independent interaction
 * - Both perspective and orthographic camera support
 *
 * The arcball center tracks the current center of interest, allowing intuitive panning
 * and smooth continuity between drag sessions.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, OrbitStyleBase
 */
class VNE_INTERACTION_API ArcballManipulator final : public OrbitStyleBase {
   public:
    /** Construct arcball manipulator with default settings. */
    ArcballManipulator() noexcept;
    /** Destroy arcball manipulator. */
    ~ArcballManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    ArcballManipulator(const ArcballManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    ArcballManipulator& operator=(const ArcballManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    ArcballManipulator(ArcballManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    ArcballManipulator& operator=(ArcballManipulator&&) noexcept = default;

    /** Check if this manipulator supports perspective projection. */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    /** Check if this manipulator supports orthographic projection. */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /** Set the camera to manipulate. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    /** Set the viewport dimensions for screen-to-world mapping. */
    void setViewportSize(float width_px, float height_px) noexcept override;
    /** Apply a semantic camera command. */
    void applyCommand(CameraActionType action,
                      const CameraCommandPayload& payload,
                      double delta_time) noexcept override;

    /** Handle mouse movement input. */
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    /** Handle mouse scroll wheel input for zoom. */
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    /** Handle touch pan gesture. */
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;

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
    /** Set the rotation damping factor for inertia (default: 4.0). */
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
    /** Compute forward direction vector from current orientation. */
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept override;
    /** Project screen pixel coordinates to a unit vector on the arcball sphere (camera-space). */
    [[nodiscard]] vne::math::Vec3f projectToArcball(float x_px, float y_px) const noexcept;
    /** Synchronize manipulator state from current camera properties. */
    void syncFromCamera() noexcept override;
    /** Apply manipulator state to update the camera. */
    void applyToCamera() noexcept override;
    /** Begin rotation from screen position. */
    void beginRotate(float x_px, float y_px) noexcept override;
    /** Handle rotation drag with pixel coordinates. */
    void dragRotate(float x_px, float y_px, double delta_time) noexcept override;
    /** End rotation and set up inertia-based momentum if enabled. */
    void endRotate(double delta_time) noexcept override;
    /** Apply rotational inertia decay based on damping. */
    void applyInertia(double delta_time) noexcept override;
    /** Called when the center of interest changes to update arcball. */
    void onPivotChanged() noexcept override;

    float arcball_start_x_ = 0.0f;  //!< Screen-space X coordinate where arcball rotation started (pixels)
    float arcball_start_y_ = 0.0f;  //!< Screen-space Y coordinate where arcball rotation started (pixels)

    vne::math::Quatf orientation_;    //!< Persistent accumulated rotation quaternion (canonical rotation state)
    uint32_t normalize_counter_ = 0;  //!< Counter for periodic quaternion normalization to prevent drift

    float inertia_rot_speed_ = 0.0f;     //!< Inertia angular velocity in radians per second
    vne::math::Vec3f inertia_rot_axis_;  //!< Inertia rotation axis in world space for momentum rotation
};

}  // namespace vne::interaction
