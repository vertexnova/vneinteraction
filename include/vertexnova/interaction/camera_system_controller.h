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

#include "vertexnova/interaction/camera_controller.h"
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/detail/camera_input_adapter.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>
#include <string>

namespace vne::interaction {

/**
 * @brief Main controller orchestrating camera input and manipulation.
 *
 * CameraSystemController is the primary API for camera control within a 3D application.
 * It owns both the active camera and the current camera manipulator (orbit, FPS, fly, etc.),
 * receives raw input events, and routes them through the manipulator to update the camera.
 *
 * Key responsibilities:
 * - Holds references to the active camera and manipulator
 * - Dispatches input events (mouse, keyboard, touch) to the current manipulator
 * - Calls update() each frame to advance animations and apply damping
 * - Supports enable/disable and reset functionality
 * - Manages input bindings for key remapping
 *
 * Typical usage:
 * ```cpp
 * auto controller = std::make_shared<CameraSystemController>();
 * controller->setCamera(my_camera);
 * controller->setManipulator(orbit_manipulator);
 *
 * // Each frame:
 * controller->handleMouseMove(x, y, dx, dy, delta_time);
 * controller->update(delta_time);
 * ```
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraController, ICameraManipulator, CameraInputBindings
 */
class VNE_INTERACTION_API CameraSystemController : public ICameraController {
   public:
    /** Construct default camera system controller. */
    CameraSystemController() = default;
    /** Destroy camera system controller. */
    ~CameraSystemController() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    CameraSystemController(const CameraSystemController&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    CameraSystemController& operator=(const CameraSystemController&) = delete;

    /** Rule of Five: default move constructor (movable). */
    CameraSystemController(CameraSystemController&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    CameraSystemController& operator=(CameraSystemController&&) noexcept = default;

    /** Set the active camera to control. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    /** Enable or disable this controller. */
    void setEnabled(bool enabled) noexcept override;
    /** Check if this controller is enabled. */
    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    /** Update the camera based on accumulated state (damping, inertia, etc.). */
    void update(double delta_time) noexcept override;
    /** Reset manipulator state and damping. */
    void reset() noexcept override;
    /** Get the currently controlled camera. */
    [[nodiscard]] std::shared_ptr<vne::scene::ICamera> getCamera() const noexcept override { return camera_; }
    /** Get the display name of this controller. */
    [[nodiscard]] const std::string& getName() const noexcept override { return name_; }
    /** Set the display name of this controller. */
    void setName(const std::string& name) noexcept override { name_ = name; }

    /** Set the camera manipulator (orbit, FPS, fly, etc.). */
    void setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept;
    [[nodiscard]] std::shared_ptr<ICameraManipulator> getManipulator() const noexcept { return manipulator_; }

    /** Set the viewport dimensions in pixels. */
    void setViewportSize(float width_px, float height_px) noexcept;
    /** Handle mouse movement input. */
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept;
    /** Handle mouse button press/release. */
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept;
    /** Handle mouse scroll wheel input. */
    void handleMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept;
    /** Handle keyboard input (key press/release). */
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept;
    /** Handle touch pan gesture. */
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept;
    /** Handle touch pinch (zoom) gesture. */
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept;

    /** Configure input key/button bindings for this controller. */
    void setInputBindings(const CameraInputBindings& bindings) noexcept;
    [[nodiscard]] CameraInputBindings getInputBindings() const noexcept;

   private:
    /** Assign the active camera to the current manipulator. */
    void assignCameraToManipulator() noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;            //!< Shared pointer to the active camera
    std::shared_ptr<ICameraManipulator> manipulator_;        //!< Shared pointer to the active manipulator
    CameraInputAdapter input_adapter_;                       //!< Adapter converting raw input to commands
    bool enabled_ = true;                                    //!< Whether this controller is enabled
    std::string name_ = "VertexNovaCameraSystemController";  //!< Display name for this controller
};

}  // namespace vne::interaction
