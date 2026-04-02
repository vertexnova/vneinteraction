#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 *
 * Autodoc:   no
 * ----------------------------------------------------------------------
 */

/**
 * @file 06_example.h
 * @brief Example 06 — custom input bindings with InputMapper and Inspect3DController.
 *
 * Demonstrates presets, bindGesture / bindScroll / bindDoubleClick / bindKey / unbindKey /
 * unbindGesture, direct InputRule construction, addRule / clearRules, direct mouse and key
 * dispatch, touch pan and pinch, and resetState on focus loss.
 */

namespace vne::interaction::examples {

[[nodiscard]] int runCustomInputBindingsExample();

}  // namespace vne::interaction::examples
