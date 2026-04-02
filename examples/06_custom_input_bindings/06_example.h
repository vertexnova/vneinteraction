/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 06: Custom input bindings — InputMapper
 *
 * Demonstrates:
 *   - All five built-in presets (orbit, fps, game, cad, ortho)
 *   - bindGesture() — rebind rotate/pan/look to a different button
 *   - bindScroll() — rebind scroll-zoom modifier
 *   - bindDoubleClick() — rebind double-click pivot action
 *   - bindKey() / unbindKey() — add/remove keyboard rules
 *   - unbindGesture() — remove an action entirely
 *   - Direct InputRule construction — full low-level control
 *   - addRule() / clearRules() workflow
 *   - resetState() on focus loss
 *   - ActionCallback wiring to CameraRig::onAction
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runCustomInputBindingsExample();

}  // namespace vne::interaction::examples
