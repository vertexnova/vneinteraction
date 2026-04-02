#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 * Autodoc:   no  (internal — excluded from install, not part of the public API)
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_controller_context.h
 * @brief Shared rig + mapper + camera + viewport + cursor for high-level controllers.
 *
 * Controllers set @ref InputMapper::setActionCallback themselves (often with a custom
 * lambda); do not store @c this-capturing callbacks on the context — moves would invalidate
 * them. Use @c Impl* capture as today.
 */

#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/input_mapper.h"

#include "controller_event_dispatch.h"

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Common state wired by Inspect / Navigation / Ortho / Follow controllers.
 */
struct CameraControllerContext {
    CameraRig rig;
    InputMapper mapper;
    std::shared_ptr<vne::scene::ICamera> camera;
    float viewport_w = 1280.0f;
    float viewport_h = 720.0f;
    CursorState cursor;

    void setCamera(std::shared_ptr<vne::scene::ICamera> cam) noexcept {
        camera = std::move(cam);
        rig.setCamera(camera);
    }

    void onResize(float width_px, float height_px) noexcept {
        viewport_w = width_px;
        viewport_h = height_px;
        rig.onResize(width_px, height_px);
    }

    void onUpdate(double delta_time) noexcept { rig.onUpdate(delta_time); }

    /** Clear cursor tracking and mapper button/key state (e.g. focus loss). */
    void resetInteraction() noexcept {
        cursor = {};
        mapper.resetState();
    }

    /** Reset manipulator state and @ref resetInteraction. */
    void resetRigAndInteraction() noexcept {
        rig.resetState();
        resetInteraction();
    }
};

}  // namespace vne::interaction
