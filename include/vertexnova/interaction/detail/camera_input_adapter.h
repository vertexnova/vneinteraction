#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @brief Translates raw input (mouse, keyboard, touch) into semantic camera
 * commands and forwards them to a manipulator.
 *
 * Decouples input bindings from manipulator logic so bindings can be remapped
 * without changing manipulators.
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>

namespace vne::interaction {

class CameraInputAdapter {
   public:
    CameraInputAdapter() = default;

    /**
     * @brief Construct adapter with a manipulator.
     * @param manipulator Shared pointer to the manipulator to forward commands to
     */
    explicit CameraInputAdapter(std::shared_ptr<ICameraManipulator> manipulator) noexcept
        : manipulator_(std::move(manipulator)) {}

    /**
     * @brief Set the manipulator to forward commands to.
     * @param manipulator Shared pointer to the manipulator (may be nullptr)
     */
    void setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept {
        manipulator_ = std::move(manipulator);
        current_action_ = Action::None;
        touch_pan_active_ = false;
    }

    /**
     * @brief Get the current manipulator.
     * @return Shared pointer to the manipulator, or nullptr
     */
    [[nodiscard]] std::shared_ptr<ICameraManipulator> getManipulator() const noexcept { return manipulator_; }

    /**
     * @brief Feed mouse movement input.
     * @param x Current cursor X in viewport pixels
     * @param y Current cursor Y in viewport pixels
     * @param delta_x Horizontal delta in pixels
     * @param delta_y Vertical delta in pixels
     * @param delta_time Time since last input in seconds
     */
    void feedMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept;

    /**
     * @brief Feed mouse button press/release.
     * @param button Mouse button index
     * @param pressed true if pressed, false if released
     * @param x Cursor X in viewport pixels
     * @param y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void feedMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept;

    /**
     * @brief Feed mouse scroll input.
     * @param scroll_x Horizontal scroll delta
     * @param scroll_y Vertical scroll delta
     * @param mouse_x Cursor X in viewport pixels
     * @param mouse_y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void feedMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept;

    /**
     * @brief Feed keyboard input.
     * @param key Key code
     * @param pressed true if pressed, false if released
     * @param delta_time Time since last input in seconds
     */
    void feedKeyboard(int key, bool pressed, double delta_time) noexcept;

    /**
     * @brief Feed touch pan gesture.
     * @param pan Pan gesture with delta_x_px and delta_y_px
     * @param delta_time Time since last input in seconds
     */
    void feedTouchPan(const TouchPan& pan, double delta_time) noexcept;

    /**
     * @brief Feed touch pinch (zoom) gesture.
     * @param pinch Pinch gesture with scale and center position
     * @param delta_time Time since last input in seconds
     */
    void feedTouchPinch(const TouchPinch& pinch, double delta_time) noexcept;

    /**
     * @brief Set input key/button bindings.
     * @param bindings New bindings configuration
     */
    void setBindings(const CameraInputBindings& bindings) noexcept { bindings_ = bindings; }

    /**
     * @brief Get the current input bindings.
     * @return Reference to the current bindings
     */
    [[nodiscard]] const CameraInputBindings& getBindings() const noexcept { return bindings_; }

   private:
    enum class Action { None, Rotate, Pan, Look };

    void send(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept;

    std::shared_ptr<ICameraManipulator> manipulator_;
    Action current_action_ = Action::None;
    float last_x_px_ = 0.0f;
    float last_y_px_ = 0.0f;
    bool modifier_shift_ = false;
    bool touch_pan_active_ = false;

    CameraInputBindings bindings_;
};

}  // namespace vne::interaction
