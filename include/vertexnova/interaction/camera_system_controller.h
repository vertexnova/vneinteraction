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
#include "vertexnova/interaction/camera_input_adapter.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/events/event.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"

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

    /**
     * @brief Set the active camera to control.
     * @param camera Shared pointer to the camera (may be nullptr)
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /**
     * @brief Enable or disable this controller.
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled) noexcept override;

    /**
     * @brief Check if this controller is enabled.
     * @return true if enabled
     */
    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }

    /**
     * @brief Update the camera based on accumulated state (damping, inertia, etc.).
     * @param delta_time Time since last update in seconds
     */
    void update(double delta_time) noexcept override;

    /**
     * @brief Reset manipulator state and damping.
     */
    void reset() noexcept override;

    /**
     * @brief Get the currently controlled camera.
     * @return Shared pointer to the active camera, or nullptr
     */
    [[nodiscard]] std::shared_ptr<vne::scene::ICamera> getCamera() const noexcept override { return camera_; }

    /**
     * @brief Get the display name of this controller.
     * @return Display name string
     */
    [[nodiscard]] const std::string& getName() const noexcept override { return name_; }

    /**
     * @brief Set the display name of this controller.
     * @param name New display name
     */
    void setName(const std::string& name) noexcept override { name_ = name; }

    /**
     * @brief Set the camera manipulator (orbit, FPS, fly, etc.).
     * @param manipulator Shared pointer to the manipulator (may be nullptr)
     */
    void setManipulator(std::shared_ptr<ICameraManipulator> manipulator) noexcept;

    /**
     * @brief Get the current camera manipulator.
     * @return Shared pointer to the active manipulator, or nullptr
     */
    [[nodiscard]] std::shared_ptr<ICameraManipulator> getManipulator() const noexcept { return manipulator_; }

    /**
     * @brief Set the viewport dimensions in pixels.
     * @param width_px Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    void setViewportSize(float width_px, float height_px) noexcept;

    /**
     * @brief Dispatch a vne::events::Event to this controller.
     *
     * Handles: eMouseMoved, eMouseButtonPressed, eMouseButtonReleased, eMouseScrolled,
     * eKeyPressed, eKeyReleased, eKeyRepeat, eTouchMove, eTouchPress, eTouchRelease.
     *
     * @param event The event to process
     * @param delta_time Time since last input in seconds
     */
    void onEvent(const vne::events::Event& event, double delta_time) noexcept;

    /**
     * @brief Configure input key/button bindings for this controller.
     * @param bindings New bindings configuration
     */
    void setInputBindings(const CameraInputBindings& bindings) noexcept;

    /**
     * @brief Get the current input bindings.
     * @return Copy of the current bindings
     */
    [[nodiscard]] CameraInputBindings getInputBindings() const noexcept;

   private:
    /**
     * @brief Assign the active camera to the current manipulator.
     */
    void assignCameraToManipulator() noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;            //!< Shared pointer to the active camera
    std::shared_ptr<ICameraManipulator> manipulator_;        //!< Shared pointer to the active manipulator
    CameraInputAdapter input_adapter_;                       //!< Adapter converting raw input to commands
    bool enabled_ = true;                                    //!< Whether this controller is enabled
    std::string name_ = "VertexNovaCameraSystemController";  //!< Display name for this controller
    double last_x_ = 0.0;                                    //!< Last known cursor X for delta computation
    double last_y_ = 0.0;                                    //!< Last known cursor Y for delta computation
    bool first_mouse_ = true;                                //!< True until first mouse event; prevents jump
};

}  // namespace vne::interaction
