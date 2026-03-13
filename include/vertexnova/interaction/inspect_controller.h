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

/**
 * @file inspect_controller.h
 * @brief InspectController — high-level camera controller for object inspection.
 *
 * The simplest way to add orbit/arcball camera interaction to a 3D viewer, CAD tool,
 * or medical 3D application. Three lines of setup:
 *
 * @code
 * auto ctrl = vne::interaction::InspectController{};
 * ctrl.setCamera(camera);
 * ctrl.setViewportSize(1920, 1080);
 * // In your loop:
 * ctrl.onEvent(event);
 * ctrl.update(dt);
 * @endcode
 *
 * ### Defaults
 * - Rotation: Arcball (quaternion) — smooth, no gimbal lock
 * - LMB drag = rotate, RMB/MMB drag = pan, scroll = zoom
 * - Double-click LMB = move pivot to cursor (auto-pivot)
 * - Pivot mode: eCoi (follows panning)
 *
 * ### Medical / CAD anchor
 * @code
 * ctrl.setPivot(landmark_world_pos);
 * ctrl.setPivotMode(OrbitPivotMode::eFixed);  // rotates around landmark
 * @endcode
 *
 * ### Disable a DOF
 * @code
 * ctrl.setRotationEnabled(false);   // pan + zoom only
 * @endcode
 *
 * ### Rebind inputs
 * @code
 * // All rules are accessible — swap or add as needed:
 * ctrl.inputMapper().setRules(myRules);
 * @endcode
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/camera_rig.h"

#include <vertexnova/math/core/core.h>

#include <memory>

namespace vne::events {
class Event;
}

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

class InputMapper;
class OrbitBehavior;

/** Rotation algorithm used by InspectController. */
enum class InspectRotationMode : std::uint8_t {
    eArcball = 0,  //!< Quaternion arcball — smooth, no gimbal lock (default)
    eOrbit   = 1,  //!< Classic Euler yaw/pitch turntable
};

/**
 * @brief High-level camera controller for object inspection.
 *
 * Wraps a CameraRig (OrbitBehavior) and an InputMapper with a sensible preset.
 * Covers: 3D model viewers, CAD, scientific visualization, medical 3D.
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API InspectController {
public:
    InspectController();
    ~InspectController();

    InspectController(const InspectController&) = delete;
    InspectController& operator=(const InspectController&) = delete;
    InspectController(InspectController&&) noexcept;
    InspectController& operator=(InspectController&&) noexcept;

    // -------------------------------------------------------------------------
    // Core setup — must call before first onEvent / update
    // -------------------------------------------------------------------------

    /** Attach a camera. Syncs internal state from its current position. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept;

    /** Notify the controller of the viewport dimensions (pixels). */
    void setViewportSize(float width_px, float height_px) noexcept;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    /** Feed a vneevents event (mouse, keyboard, touch, double-click). */
    void onEvent(const vne::events::Event& event) noexcept;

    /** Advance inertia and fit animation by delta_time seconds. */
    void update(double delta_time) noexcept;

    // -------------------------------------------------------------------------
    // Rotation
    // -------------------------------------------------------------------------

    /** Switch rotation algorithm (default: eArcball). */
    void setRotationMode(InspectRotationMode mode) noexcept;
    [[nodiscard]] InspectRotationMode getRotationMode() const noexcept { return rotation_mode_; }

    // -------------------------------------------------------------------------
    // Pivot / anchor
    // -------------------------------------------------------------------------

    /**
     * @brief Set the orbit pivot to a fixed world-space position.
     * Automatically switches pivot mode to OrbitPivotMode::eFixed.
     * Use for medical landmarks, CAD reference points, etc.
     */
    void setPivot(const vne::math::Vec3f& world_pos) noexcept;

    /**
     * @brief Control how the pivot point behaves.
     * - eCoi    — pivot follows panning (default)
     * - eFixed  — pivot stays at the point set by setPivot()
     * - eViewCenter — pivot moves to screen-center on pan end
     */
    void setPivotMode(OrbitPivotMode mode) noexcept;
    [[nodiscard]] OrbitPivotMode getPivotMode() const noexcept;

    // -------------------------------------------------------------------------
    // DOF enable/disable (delegates to InputMapper rule removal)
    // -------------------------------------------------------------------------

    /** Enable or disable rotation (removes/restores rotate rules). */
    void setRotationEnabled(bool enabled) noexcept;
    /** Enable or disable panning (removes/restores pan rules). */
    void setPanEnabled(bool enabled) noexcept;
    /** Enable or disable zoom (removes/restores zoom rules). */
    void setZoomEnabled(bool enabled) noexcept;

    // -------------------------------------------------------------------------
    // Convenience
    // -------------------------------------------------------------------------

    /** Fit camera to an AABB with smooth animation. */
    void fitToAABB(const vne::math::Vec3f& min_world,
                   const vne::math::Vec3f& max_world) noexcept;

    /** Reset camera and interaction state. */
    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches for power users
    // -------------------------------------------------------------------------

    /** Direct access to the underlying InputMapper for full rebind. */
    [[nodiscard]] InputMapper& inputMapper() noexcept;

    /** Direct access to the underlying OrbitBehavior for fine-tuning. */
    [[nodiscard]] OrbitBehavior& orbitBehavior() noexcept;

private:
    void rebuildRules() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;

    InspectRotationMode rotation_mode_ = InspectRotationMode::eArcball;
    bool rotation_enabled_ = true;
    bool pan_enabled_      = true;
    bool zoom_enabled_     = true;
};

}  // namespace vne::interaction
