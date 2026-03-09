#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Input simulation helpers for VneInteraction examples.
 * All functions drive an ICameraManipulator programmatically —
 * no window or GPU required.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"

#include <initializer_list>

namespace vne::interaction::examples {

/**
 * Press a mouse button, drag N frames distributing (total_dx, total_dy)
 * evenly across frames, then release.
 *
 * @param total_dx  Total X displacement across all frames (e.g. 30 frames
 *                  × 2 px/frame = pass 60.0f here).
 * @param total_dy  Total Y displacement across all frames.
 */
inline void simulateMouseDrag(ICameraManipulator& manipulator,
                              MouseButton button,
                              float start_x,
                              float start_y,
                              float total_dx,
                              float total_dy,
                              int frames,
                              double dt) {
    const int btn = static_cast<int>(button);
    const float step_x = (frames > 0) ? (total_dx / static_cast<float>(frames)) : 0.0f;
    const float step_y = (frames > 0) ? (total_dy / static_cast<float>(frames)) : 0.0f;

    manipulator.handleMouseButton(btn, /*pressed=*/true, start_x, start_y, dt);

    float cx = start_x;
    float cy = start_y;
    for (int i = 0; i < frames; ++i) {
        manipulator.handleMouseMove(cx + step_x, cy + step_y, step_x, step_y, dt);
        cx += step_x;
        cy += step_y;
    }

    manipulator.handleMouseButton(btn, /*pressed=*/false, cx, cy, dt);
}

/**
 * Fire @p count scroll events at the given cursor position.
 *
 * @param scroll_y  Amount per event. Negative = zoom in (library convention).
 */
inline void simulateMouseScroll(
    ICameraManipulator& manipulator, float scroll_y, float mouse_x, float mouse_y, int count, double dt) {
    for (int i = 0; i < count; ++i) {
        manipulator.handleMouseScroll(0.0f, scroll_y, mouse_x, mouse_y, dt);
    }
}

/**
 * Press a key, run @p frames update() ticks, then release.
 */
inline void simulateKeyHold(ICameraManipulator& manipulator, int key, int frames, double dt) {
    manipulator.handleKeyboard(key, /*pressed=*/true, dt);
    for (int i = 0; i < frames; ++i) {
        manipulator.update(dt);
    }
    manipulator.handleKeyboard(key, /*pressed=*/false, dt);
}

/**
 * Press multiple keys simultaneously, run @p frames update() ticks,
 * then release all keys in reverse insertion order (LIFO).
 *
 * Example — sprint-forward (Shift+W):
 *   simulateKeyHoldMulti(manip, {key_shift, key_w}, 30, dt);
 */
inline void simulateKeyHoldMulti(ICameraManipulator& manipulator,
                                 std::initializer_list<int> keys,
                                 int frames,
                                 double dt) {
    for (int k : keys) {
        manipulator.handleKeyboard(k, /*pressed=*/true, dt);
    }
    for (int i = 0; i < frames; ++i) {
        manipulator.update(dt);
    }
    // Release in reverse order to mirror natural key-up sequencing
    for (auto it = keys.end(); it != keys.begin();) {
        --it;
        manipulator.handleKeyboard(*it, /*pressed=*/false, dt);
    }
}

/**
 * Run @p frames update() ticks — used for inertia decay after input release.
 */
inline void runFrames(ICameraManipulator& manipulator, int frames, double dt) {
    for (int i = 0; i < frames; ++i) {
        manipulator.update(dt);
    }
}

}  // namespace vne::interaction::examples
