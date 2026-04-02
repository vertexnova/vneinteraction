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
 * @file ortho_2d_controller.h
 * @brief Ortho2DController — high-level camera controller for orthographic 2D viewports.
 *
 * Designed for 2D maps, DICOM slice viewers, sprite editors, and any orthographic
 * viewport where panning and zoom-at-cursor are the primary interactions.
 *
 * @code
 * auto ctrl = vne::interaction::Ortho2DController{};
 * ctrl.setCamera(orthoCamera);
 * ctrl.onResize(512, 512);
 * // Scroll = zoom-at-cursor, LMB/MMB drag = pan. Done.
 * @endcode
 *
 * ### Enable rotation (e.g. for slice reorientation)
 * @code
 * ctrl.setRotationEnabled(true);  // RMB drag: in-plane rotation (Ortho2DManipulator)
 * @endcode
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/camera_controller.h"
#include "vertexnova/interaction/camera_rig.h"

#include <vertexnova/events/types.h>
#include <memory>

namespace vne::events {
class Event;
}

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

class InputMapper;
class Ortho2DManipulator;

/**
 * @brief High-level camera controller for orthographic 2D viewports.
 *
 * Wraps a CameraRig (Ortho2DManipulator) and an InputMapper with the ortho preset.
 * Rotation is disabled by default; enable with setRotationEnabled(true).
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API Ortho2DController : public ICameraController {
   public:
    Ortho2DController();
    ~Ortho2DController();

    Ortho2DController(const Ortho2DController&) = delete;
    Ortho2DController& operator=(const Ortho2DController&) = delete;
    Ortho2DController(Ortho2DController&&) noexcept;
    Ortho2DController& operator=(Ortho2DController&&) noexcept;

    // -------------------------------------------------------------------------
    // Core setup
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void onResize(float width_px, float height_px) noexcept override;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept override;
    void onUpdate(double delta_time) noexcept override;

    // -------------------------------------------------------------------------
    // DOF
    // -------------------------------------------------------------------------

    /**
     * @brief Enable or disable in-plane rotation (default: disabled).
     * When enabled, RMB drag rotates the view around the screen center.
     */
    void setRotationEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isRotationEnabled() const noexcept { return rotation_enabled_; }

    /** Enable or disable panning (default: enabled). */
    void setPanEnabled(bool enabled) noexcept;

    /** Enable or disable zoom (default: enabled). */
    void setZoomEnabled(bool enabled) noexcept;

    /** Rebind pan primary button (default LMB). */
    void setPanButton(MouseButton btn, vne::events::ModifierKey modifier = vne::events::ModifierKey::eModNone) noexcept;
    /** Rebind rotate button (default RMB when rotation enabled). */
    void setRotateButton(MouseButton btn,
                         vne::events::ModifierKey modifier = vne::events::ModifierKey::eModNone) noexcept;
    /** Scroll zoom modifier requirement (default none). */
    void setZoomScrollModifier(vne::events::ModifierKey modifier) noexcept;

    /** Delegate to Ortho2DManipulator::setPanDamping. */
    void setPanInertiaEnabled(bool enabled) noexcept;
    /** Delegate to Ortho2DManipulator::setRotationSensitivityDegreesPerPixel. */
    void setRotateSensitivity(float degrees_per_pixel) noexcept;
    /** Delegate to Ortho2DManipulator::setZoomSpeed. */
    void setZoomSensitivity(float multiplier) noexcept;
    /** Delegate to Ortho2DManipulator::setPanDamping. */
    void setPanSensitivity(float damping) noexcept;

    // -------------------------------------------------------------------------
    // Convenience
    // -------------------------------------------------------------------------

    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches
    // -------------------------------------------------------------------------

    [[nodiscard]] InputMapper& inputMapper() noexcept;
    [[nodiscard]] Ortho2DManipulator& ortho2DManipulator() noexcept;

   private:
    void rebuildRules() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;

    bool rotation_enabled_ = false;
    bool pan_enabled_ = true;
    bool zoom_enabled_ = true;
    MouseBinding pan_binding_{MouseButton::eLeft, vne::events::ModifierKey::eModNone};
    MouseBinding rotate_binding_{MouseButton::eRight, vne::events::ModifierKey::eModNone};
    vne::events::ModifierKey zoom_scroll_modifier_ = vne::events::ModifierKey::eModNone;
};

}  // namespace vne::interaction
