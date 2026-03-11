#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <memory>

namespace vne::scene {
class ICamera;
class PerspectiveCamera;
class OrthographicCamera;
}  // namespace vne::scene

namespace vne::interaction {

/**
 * @brief Base for all camera manipulators.
 *
 * Holds common state and provides default implementations for setEnabled,
 * getSceneScale, setViewportSize. Exported so virtuals are visible across
 * the DLL boundary (e.g. when linking tests).
 */
class VNE_INTERACTION_API CameraManipulatorBase : public ICameraManipulator {
   protected:
    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    float scene_scale_ = 1.0f;
    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
    float zoom_speed_ = 1.1f;

    CameraManipulatorBase() = default;
    ~CameraManipulatorBase() override = default;

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> perspCamera() const noexcept;
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> orthoCamera() const noexcept;

   public:
    /**
     * @brief Enable or disable this manipulator.
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

    /**
     * @brief Check if this manipulator is enabled.
     * @return true if enabled
     */
    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }

    /**
     * @brief Get the current scene scale value.
     * @return Scene scale factor
     */
    [[nodiscard]] float getSceneScale() const noexcept override { return scene_scale_; }

    /**
     * @brief Get the zoom speed for scroll/pinch events.
     * @return Zoom speed factor
     */
    [[nodiscard]] float getZoomSpeed() const noexcept override { return zoom_speed_; }

    /**
     * @brief Set the viewport dimensions in pixels.
     * @param width_px Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    void setViewportSize(float width_px, float height_px) noexcept override;
    void applyCommand(CameraActionType, const CameraCommandPayload&, double) noexcept override {}
};

}  // namespace vne::interaction
