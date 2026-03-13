#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   no  (internal — not part of the public API)
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_behavior_base.h
 * @brief CameraBehaviorBase — internal non-virtual base for ICameraBehavior implementations.
 *
 * Provides the common fields and trivial method bodies shared by every concrete
 * behavior (camera_, enabled_, viewport_width_, viewport_height_, scene_scale_).
 * This header is internal: it lives under src/ and is never installed.
 */

#include "vertexnova/interaction/camera_behavior.h"

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Internal non-virtual base class for ICameraBehavior implementations.
 *
 * Concrete behaviors inherit from this instead of ICameraBehavior directly.
 * They still override the remaining pure-virtual methods (onAction, onUpdate,
 * resetState) and may override setCamera / setViewportSize when extra sync work
 * is needed (calling the base version first).
 *
 * Not part of the public API — do not include from public headers.
 */
class CameraBehaviorBase : public ICameraBehavior {
   public:
    ~CameraBehaviorBase() noexcept override = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior — implemented here so concrete classes don't repeat them
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override { camera_ = std::move(camera); }

    void setViewportSize(float width_px, float height_px) noexcept override {
        viewport_width_ = width_px;
        viewport_height_ = height_px;
    }

    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

   protected:
    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;

    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    float scene_scale_ = 1.0f;
};

}  // namespace vne::interaction
