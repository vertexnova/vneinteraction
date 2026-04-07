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
 * @file camera_manipulator_base.h
 * @brief CameraManipulatorBase — base implementation of ICameraManipulator with shared zoom-dispatch logic.
 *
 * Provides common fields, trivial method bodies, and shared zoom dispatch
 * logic used by every concrete manipulator. The zoom dispatch uses the template
 * method pattern: dispatchZoom() is non-virtual and handles eChangeFov and
 * eSceneScale internally; eDollyToCoi is routed to the virtual applyDolly()
 * hook — the default implementation handles orthographic zoom-to-cursor;
 * concrete manipulators override it to implement perspective dolly.
 *
 * This header is part of the public interaction API surface.
 */

#include "vertexnova/interaction/camera_manipulator.h"

#include <vertexnova/math/core/types.h>
#include <vertexnova/math/viewport.h>

#include <algorithm>
#include <memory>

namespace vne::scene {
class ICamera;
class PerspectiveCamera;
class OrthographicCamera;
}  // namespace vne::scene

namespace vne::interaction {

/**
 * @brief Base class for ICameraManipulator implementations with shared zoom logic and virtual applyDolly().
 *
 * Concrete manipulators inherit from this instead of ICameraManipulator directly.
 * They still override the remaining pure-virtual methods (onAction, onUpdate,
 * resetState) and may override setCamera / onResize when extra sync work
 * is needed (calling the base version first).
 *
 * ## Zoom dispatch
 *
 * Call `dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px)` from
 * the `eZoomAtCursor` case of `onAction`. The base class handles eChangeFov and
 * eSceneScale centrally. For eDollyToCoi it calls the virtual `applyDolly()` —
 * override that to implement manipulator-specific perspective dolly. The default
 * applyDolly handles orthographic zoom-to-cursor automatically, so
 * Ortho2DManipulator needs no override.
 */
class VNE_INTERACTION_API CameraManipulatorBase : public ICameraManipulator {
   public:
    ~CameraManipulatorBase() noexcept override = default;

    // -------------------------------------------------------------------------
    // ICameraManipulator — implemented here so concrete classes don't repeat them
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    void onResize(float width_px, float height_px) noexcept override {
        viewport_.width = std::max(1.0f, width_px);
        viewport_.height = std::max(1.0f, height_px);
    }

    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

    // -------------------------------------------------------------------------
    // Zoom method API — shared across all manipulators
    // -------------------------------------------------------------------------

    /** Set the zoom interaction method (dolly, scene scale, or FOV). */
    void setZoomMethod(ZoomMethod method) noexcept;
    /** Get the current zoom interaction method. */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /**
     * @brief Reserved tuning knob (>= 0.01). eChangeFov uses scroll/pinch factor magnitude
     * directly on FOV / ortho extents; eSceneScale multiplies accumulated scale by the event factor.
     */
    void setFovZoomSpeed(float speed) noexcept;
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }

    /**
     * @brief Get the accumulated zoom scale factor (meaningful for eSceneScale).
     * Returns 1.0 when no scaling has been applied.
     */
    [[nodiscard]] float getZoomScale() const noexcept { return zoom_scale_; }

   protected:
    // -------------------------------------------------------------------------
    // Shared constants
    // -------------------------------------------------------------------------

    static constexpr float kFovMinDeg = 5.0f;
    static constexpr float kFovMaxDeg = 160.0f;
    static constexpr float kSceneScaleMin = 1e-4f;
    static constexpr float kSceneScaleMax = 1e4f;
    static constexpr float kMinOrthoExtent = 1e-3f;

    // -------------------------------------------------------------------------
    // Camera type helpers
    // -------------------------------------------------------------------------

    /** @brief Return camera cast to PerspectiveCamera, or nullptr. */
    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> perspCamera() const noexcept;
    /** @brief Return camera cast to OrthographicCamera, or nullptr. */
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> orthoCamera() const noexcept;

    /** @brief Viewport dimensions (updated by onResize). */
    [[nodiscard]] const vne::math::Viewport& viewport() const noexcept { return viewport_; }
    [[nodiscard]] float viewportWidth() const noexcept { return viewport_.width; }
    [[nodiscard]] float viewportHeight() const noexcept { return viewport_.height; }
    /** @brief Graphics API for screen/NDC conventions (from camera, or eOpenGL if none). */
    [[nodiscard]] vne::math::GraphicsApi graphicsApi() const noexcept;

    // -------------------------------------------------------------------------
    // Zoom dispatch (template method pattern)
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a zoom event to the appropriate implementation.
     *
     * Call from the eZoomAtCursor case of onAction:
     * @code
     *   dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px);
     * @endcode
     *
     * Routing:
     *  - eSceneScale → applySceneScaleZoom(factor)
     *  - eChangeFov  → applyFovZoom(factor)
     *  - eDollyToCoi → applyDolly(factor, mx, my)
     *
     * @param factor  Zoom factor (< 1 = zoom in, > 1 = zoom out, must be > 0)
     * @param mx      Mouse X in pixels (for cursor-anchored zoom)
     * @param my      Mouse Y in pixels (for cursor-anchored zoom)
     */
    void dispatchZoom(float factor, float mx, float my) noexcept;

    /**
     * @brief Geometric zoom for eDollyToCoi (orbit dolly, ortho zoom-to-cursor, etc.).
     *
     * Default: handles orthographic cameras via applyOrthoZoomToCursor().
     * For perspective cameras the default is a no-op — override to implement
     * perspective dolly.
     *
     * When overriding for a manipulator that also supports ortho, call the base
     * in the ortho branch:
     * @code
     *   if (auto ortho = orthoCamera()) {
     *       CameraManipulatorBase::applyDolly(factor, mx, my);  // handles ortho
     *       coi_world_ = ortho->getTarget();  // sync orbit-specific state
     *       return;
     *   }
     *   // ... perspective dolly logic ...
     * @endcode
     */
    virtual void applyDolly(float factor, float mx, float my) noexcept;

    // -------------------------------------------------------------------------
    // Shared zoom implementations
    // -------------------------------------------------------------------------

    /**
     * @brief FOV / ortho-extent zoom using the event factor magnitude.
     *
     * Perspective: FOV *= factor, clamped to [kFovMinDeg, kFovMaxDeg].
     * Orthographic: half-extents *= factor with uniform scale and ortho half clamps.
     *
     * @param factor Per-event multiplier (< 1 = zoom in, > 1 = zoom out).
     */
    void applyFovZoom(float factor) noexcept;

    /**
     * @brief Scene-scale zoom — accumulates zoom_scale_, syncs ICamera::setSceneScale, refreshes matrices.
     *
     * @param factor Multiplier applied to accumulated zoom_scale_ (clamped to [kSceneScaleMin, kSceneScaleMax]).
     */
    void applySceneScaleZoom(float factor) noexcept;

    /**
     * @brief Orthographic cursor-anchored zoom.
     *
     * Scales the orthographic frustum bounds so the world point under the cursor
     * remains stationary. factor > 1 enlarges the view (zoom out), < 1 shrinks it.
     *
     * @param factor  Zoom factor
     * @param mx      Mouse X in pixels
     * @param my      Mouse Y in pixels
     */
    void applyOrthoZoomToCursor(float factor, float mx, float my) noexcept;

    // -------------------------------------------------------------------------
    // Shared state
    // -------------------------------------------------------------------------

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;

    vne::math::Viewport viewport_{1280.0f, 720.0f};

    ZoomMethod zoom_method_ = ZoomMethod::eSceneScale;
    float zoom_scale_ = 1.0f;       //!< Accumulated zoom scale (eSceneScale)
    float fov_zoom_speed_ = 1.05f;  //!< Legacy default for setFovZoomSpeed (eChangeFov uses event factor)
};

}  // namespace vne::interaction
