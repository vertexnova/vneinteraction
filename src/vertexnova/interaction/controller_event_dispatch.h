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
 * @file controller_event_dispatch.h
 * @brief dispatchMouseEvents — shared mouse/touch event→InputMapper bridge.
 *
 * Eliminates the verbatim copy of the mouse/touch switch-case that appeared
 * in InspectController, Navigation3DController, and Ortho2DController.
 * Internal header; not installed.
 */

#include "vertexnova/interaction/input_mapper.h"

namespace vne::events {
class Event;
}

namespace vne::interaction {

/**
 * @brief Cursor tracking state shared across mouse and touch events.
 *
 * Each controller Impl embeds one of these instead of raw first_mouse/last_x/last_y fields.
 */
struct CursorState {
    bool first = true;
    double last_x = 0.0;
    double last_y = 0.0;
};

/**
 * @brief Dispatch mouse and touch events to an InputMapper.
 *
 * Handles: eMouseMoved, eMouseButtonPressed, eMouseButtonReleased,
 *          eMouseButtonDoubleClicked, eMouseScrolled,
 *          eTouchPress, eTouchMove, eTouchRelease.
 *
 * Key events are NOT handled here — Navigation3DController processes those itself.
 *
 * @param mapper   The InputMapper to forward translated events to
 * @param cursor   Mutable cursor tracking state (updated in-place)
 * @param event    The incoming event
 * @param dt       Delta time in seconds (pass 0.0 when not available)
 */
void dispatchMouseEvents(InputMapper& mapper, CursorState& cursor, const vne::events::Event& event, double dt) noexcept;

}  // namespace vne::interaction
