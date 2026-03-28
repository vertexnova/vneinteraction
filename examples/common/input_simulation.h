#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Input simulation helpers for VneInteraction examples.
 * Drives event-based controllers (Inspect3DController, Navigation3DController,
 * Ortho2DController) via vne::events — no window or GPU required.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/events/event.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"

#include <functional>
#include <initializer_list>

namespace vne::interaction::examples {

/** Callback: (event, delta_time) — used to feed events to a controller. */
using EventCallback = std::function<void(const vne::events::Event&, double)>;

/**
 * Press a mouse button, drag N frames distributing (total_dx, total_dy)
 * evenly across frames, then release.
 */
inline void simulateMouseDrag(EventCallback on_event,
                              vne::events::MouseButton button,
                              float start_x,
                              float start_y,
                              float total_dx,
                              float total_dy,
                              int frames,
                              double dt) {
    const float step_x = (frames > 0) ? (total_dx / static_cast<float>(frames)) : 0.0f;
    const float step_y = (frames > 0) ? (total_dy / static_cast<float>(frames)) : 0.0f;

    vne::events::MouseButtonPressedEvent press(button, 0, static_cast<double>(start_x), static_cast<double>(start_y));
    on_event(press, dt);

    float cx = start_x;
    float cy = start_y;
    for (int i = 0; i < frames; ++i) {
        cx += step_x;
        cy += step_y;
        vne::events::MouseMovedEvent move(static_cast<double>(cx), static_cast<double>(cy));
        on_event(move, dt);
    }

    vne::events::MouseButtonReleasedEvent release(button, 0, static_cast<double>(cx), static_cast<double>(cy));
    on_event(release, dt);
}

/**
 * Fire @p count scroll events at the given cursor position.
 * Sets cursor position via MouseMoved first (controllers use last position for zoom center).
 * @param scroll_y  Amount per event. Negative = zoom in (library convention).
 */
inline void simulateMouseScroll(
    EventCallback on_event, float scroll_y, float mouse_x, float mouse_y, int count, double dt) {
    vne::events::MouseMovedEvent pos(static_cast<double>(mouse_x), static_cast<double>(mouse_y));
    on_event(pos, dt);
    for (int i = 0; i < count; ++i) {
        vne::events::MouseScrolledEvent evt(0.0, static_cast<double>(scroll_y));
        on_event(evt, dt);
    }
}

/**
 * Press a key, run @p frames update ticks, then release.
 * Caller must call controller.onUpdate(dt) in the loop.
 */
inline void simulateKeyHold(
    EventCallback on_event, vne::events::KeyCode key, int frames, double dt, std::function<void(double)> update_fn) {
    vne::events::KeyPressedEvent press(key);
    on_event(press, dt);
    for (int i = 0; i < frames; ++i) {
        update_fn(dt);
    }
    vne::events::KeyReleasedEvent release(key);
    on_event(release, dt);
}

/**
 * Run @p frames update ticks — used for inertia decay after input release.
 */
inline void runFrames(std::function<void(double)> update_fn, int frames, double dt) {
    for (int i = 0; i < frames; ++i) {
        update_fn(dt);
    }
}

}  // namespace vne::interaction::examples
