#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera.h"
#include <memory>

namespace vne::interaction {

class ICameraManipulator {
public:
    virtual ~ICameraManipulator() noexcept = default;

    [[nodiscard]] virtual bool supportsPerspective() const noexcept = 0;
    [[nodiscard]] virtual bool supportsOrthographic() const noexcept = 0;

    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;
    virtual void setEnabled(bool enabled) noexcept = 0;
    virtual void setViewportSize(float width_px, float height_px) noexcept = 0;
    virtual void update(double delta_time) noexcept = 0;

    virtual void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept = 0;
    virtual void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept = 0;
    virtual void handleMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept = 0;
    virtual void handleKeyboard(int key, bool pressed, double delta_time) noexcept = 0;
    virtual void handleTouchPan(const TouchPan& pan, double delta_time) noexcept = 0;
    virtual void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept = 0;

    [[nodiscard]] virtual float getSceneScale() const noexcept { return 1.0f; }
};

}  // namespace vne::interaction
