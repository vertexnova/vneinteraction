#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>

namespace vne::interaction {

/**
 * @brief Thin controller that holds the current manipulator and forwards input.
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API CameraSystemController {
   public:
    CameraSystemController() = default;
    ~CameraSystemController() = default;

    void setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept {
        manipulator_ = std::move(manipulator);
    }
    [[nodiscard]] std::shared_ptr<ICameraManipulator> getManipulator() const noexcept { return manipulator_; }

    void setViewportSize(float width_px, float height_px) noexcept;
    void update(double delta_time) noexcept;
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept;
    void handleMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept;

   private:
    std::shared_ptr<ICameraManipulator> manipulator_;
};

}  // namespace vne::interaction
