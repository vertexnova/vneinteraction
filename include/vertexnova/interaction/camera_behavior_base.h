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
 * @file camera_behavior_base.h
 * @brief CameraBehaviorBase — base implementation of ICameraBehavior with shared zoom-dispatch logic.
 *
 * Provides common fields, trivial method bodies, and shared zoom dispatch
 * logic used by every concrete behavior. The zoom dispatch uses the template
 * method pattern: dispatchZoom() is non-virtual and handles eChangeFov and
 * eSceneScale internally; eDollyToCoi is routed to the virtual onZoomDolly()
 * hook — the default implementation handles orthographic zoom-to-cursor;
 * concrete behaviors override it to implement perspective dolly.
 *
 * This header is part of the public interaction API surface.
 */

#include "vertexnova/interaction/camera_behavior.h"

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
 * @brief Base class for ICameraBehavior implementations with shared zoom logic and a virtual dolly-zoom hook.
 *
 * Concrete behaviors inherit from this instead of ICameraBehavior directly.
 * They still override the remaining pure-virtual methods (onAction, onUpdate,
 * resetState) and may override setCamera / onResize when extra sync work
 * is needed (calling the base version first).
 *
 * ## Zoom dispatch
 *
 * Call `dispatchZoom(payload.zoom_factor, payload.x_px, payload.y_px)` from
 * the `eZoomAtCursor` case of `onAction`. The base class handles eChangeFov and
 * eSceneScale centrally. For eDollyToCoi it calls the virtual `onZoomDolly()` —
 * override that to implement behavior-specific perspective dolly. The default
 * onZoomDolly handles orthographic zoom-to-cursor automatically, so
 * OrthoPanZoomBehavior needs no override.
 */
class VNE_INTERACTION_API CameraBehaviorBase : public ICameraBehavior {
   public:
    ~CameraBehaviorBase() noexcept override = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior — implemented here so concrete classes don't repeat them
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    void onResize(float width_px, float height_px) noexcept override {
        viewport_.width = std::max(1.0f, width_px);
        viewport_.height = std::max(1.0f, height_px);
    }

    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

    // -------------------------------------------------------------------------
    // Zoom method API — shared across all behaviors
    // -------------------------------------------------------------------------

    /** Set the zoom interaction method (dolly, scene scale, or FOV). */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    /** Get the current zoom interaction method. */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /**
     * @brief Set the step rate for FOV and scene-scale zoom (>= 0.01).
     *
     * For eChangeFov: FOV is divided by this value on zoom-in and multiplied
     * by it on zoom-out.
     * For eSceneScale: the accumulated scale is multiplied/divided by this value.
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
    static constexpr float kFovMaxDeg = 120.0f;
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
     *  - eChangeFov  → applyFovZoom(factor)
     *  - eSceneScale → applySceneScaleZoom(factor)
     *  - eDollyToCoi → onZoomDolly(factor, mx, my)
     *
     * @param factor  Zoom factor (< 1 = zoom in, > 1 = zoom out, must be > 0)
     * @param mx      Mouse X in pixels (for cursor-anchored zoom)
     * @param my      Mouse Y in pixels (for cursor-anchored zoom)
     */
    void dispatchZoom(float factor, float mx, float my) noexcept;

    /**
     * @brief Dolly-zoom hook, called by dispatchZoom for eDollyToCoi.
     *
     * Default: handles orthographic cameras via applyOrthoZoomToCursor().
     * For perspective cameras the default is a no-op — override to implement
     * perspective dolly.
     *
     * When overriding for a behavior that also supports ortho, call the base
     * in the ortho branch:
     * @code
     *   if (auto ortho = orthoCamera()) {
     *       CameraBehaviorBase::onZoomDolly(factor, mx, my);  // handles ortho
     *       coi_world_ = ortho->getTarget();  // sync orbit-specific state
     *       return;
     *   }
     *   // ... perspective dolly logic ...
     * @endcode
     */
    virtual void onZoomDolly(float factor, float mx, float my) noexcept;

    // -------------------------------------------------------------------------
    // Shared zoom implementations
    // -------------------------------------------------------------------------

    /**
     * @brief FOV zoom — applies fov_zoom_speed_ in the direction given by factor.
     *
     * Perspective: FOV multiplied or divided by fov_zoom_speed_, clamped to
     *   [kFovMinDeg, kFovMaxDeg]. No fallthrough.
     * Orthographic: vertical half-extent scaled by fov_zoom_speed_, aspect
     *   ratio preserved, clamped to kMinOrthoExtent.
     *
     * @param factor Direction: < 1 = zoom in (reduce FOV), > 1 = zoom out.
     */
    void applyFovZoom(float factor) noexcept;

    /**
     * @brief Scene-scale zoom — accumulates zoom_scale_ for getZoomScale() / app use.
     *
     * Accumulates zoom_scale_ by multiplying with factor (clamped to
     * [kSceneScaleMin, kSceneScaleMax]). ICamera does not expose scene scale; callers
     * may read getZoomScale() and apply scale in their pipeline if needed.
     *
     * @param factor Multiplier applied to accumulated zoom_scale_.
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

    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
    float zoom_scale_ = 1.0f;       //!< Accumulated zoom scale (eSceneScale)
    float fov_zoom_speed_ = 1.05f;  //!< Multiplicative step for FOV/scene-scale zoom
};

}  // namespace vne::interaction
