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
 * @file inspect_3d_controller.h
 * @brief Inspect3DController — high-level camera controller for object inspection.
 *
 * The simplest way to add orbit / trackball camera interaction to a 3D viewer, CAD tool,
 * or medical 3D application. Three lines of setup:
 *
 * @code
 * auto ctrl = vne::interaction::Inspect3DController{};
 * ctrl.setCamera(camera);
 * ctrl.onResize(1920, 1080);
 * // In your loop:
 * ctrl.onEvent(event);
 * ctrl.onUpdate(dt);
 * @endcode
 *
 * ### Defaults
 * - Rotation: **off** by default; when enabled, algorithm is **Euler orbit** (`OrbitRotationMode::eOrbit`).
 *   Use `setRotationMode(OrbitRotationMode::eTrackball)` for quaternion trackball.
 * - LMB drag = rotate (when rotation enabled), RMB/MMB drag = pan, scroll = zoom
 * - Double-click LMB = move pivot to cursor (auto-pivot)
 * - Pivot mode: eCoi (follows panning)
 *
 * ### Medical / CAD anchor
 * @code
 * ctrl.setPivot(landmark_world_pos);
 * ctrl.setPivotMode(OrbitPivotMode::eFixed);  // rotates around landmark
 * @endcode
 *
 * ### Enable rotation
 * @code
 * ctrl.setRotationEnabled(true);   // LMB orbit / trackball (see setRotationMode)
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
class OrbitalCameraBehavior;

/**
 * @brief High-level camera controller for object inspection.
 *
 * Wraps a CameraRig (OrbitalCameraBehavior) and an InputMapper with a sensible preset.
 * Covers: 3D model viewers, CAD, scientific visualization, medical 3D.
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API Inspect3DController {
   public:
    Inspect3DController();
    ~Inspect3DController();

    Inspect3DController(const Inspect3DController&) = delete;
    Inspect3DController& operator=(const Inspect3DController&) = delete;
    Inspect3DController(Inspect3DController&&) noexcept;
    Inspect3DController& operator=(Inspect3DController&&) noexcept;

    // -------------------------------------------------------------------------
    // Core setup — must call before first onEvent / onUpdate
    // -------------------------------------------------------------------------

    /** Attach a camera. Syncs internal state from its current position. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept;

    /** Notify the controller of the viewport dimensions (pixels). */
    void onResize(float width_px, float height_px) noexcept;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    /** Feed a vneevents event (mouse, keyboard, touch, double-click). */
    void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept;

    /** Advance inertia and fit animation by delta_time seconds. */
    void onUpdate(double delta_time) noexcept;

    // -------------------------------------------------------------------------
    // Rotation
    // -------------------------------------------------------------------------

    /** Switch rotation algorithm (default: OrbitRotationMode::eOrbit). */
    void setRotationMode(OrbitRotationMode mode) noexcept;
    [[nodiscard]] OrbitRotationMode getRotationMode() const noexcept;

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
     * @brief Control how the pivot point behaves (see @ref OrbitPivotMode).
     * - eCoi — orbit center follows pan in the view plane (default)
     * - eFixed — world pivot fixed; pan trucks eye+target (e.g. after @ref setPivot)
     * - eViewCenter — same as eCoi while panning; on pan end, COI syncs from the camera target
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
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

    /** Reset camera and interaction state. */
    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches for power users
    // -------------------------------------------------------------------------

    /** Direct access to the underlying InputMapper for full rebind. */
    [[nodiscard]] InputMapper& inputMapper() noexcept;

    /** Direct access to the underlying OrbitalCameraBehavior for fine-tuning. */
    [[nodiscard]] OrbitalCameraBehavior& orbitalCameraBehavior() noexcept;

   private:
    void rebuildRules() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vne::interaction
