#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * GLFW key code constants for use in VneInteraction examples.
 * Values match GLFW key definitions (platform-independent ASCII subset).
 * ----------------------------------------------------------------------
 */

namespace vne::interaction::examples {

// Movement keys (GLFW key codes match ASCII for A–Z)
constexpr int key_a = 65;
constexpr int key_d = 68;
constexpr int key_e = 69;
constexpr int key_q = 81;
constexpr int key_s = 83;
constexpr int key_w = 87;

// Modifier keys
constexpr int key_shift = 340;  // GLFW_KEY_LEFT_SHIFT (right shift = 344)

}  // namespace vne::interaction::examples
