# 06 — Custom Input Bindings

Shows every `InputMapper` customization API from preset selection down to raw `InputRule` construction.

## What it covers

- **All five presets** (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`) — rule counts confirm they're non-empty
- **Controller binding API** — `Inspect3DController::setRotateButton`, `setPanButton`, `setZoomScrollModifier`, `setPivotOnDoubleClickEnabled` rebuild rules without touching `InputMapper` directly
- **`bindGesture`** — reassign rotate / pan / look to a different mouse button
- **`bindScroll`** — require a modifier (e.g. Alt) before scroll zooms
- **`bindDoubleClick`** — change the double-click pivot button
- **`unbindGesture`** — remove an action entirely (pen-tablet workflow with no scroll)
- **`bindKey` / `unbindKey`** — add Space=jump, remove W-forward
- **Direct `InputRule` construction** — build a minimal orbit rule set from scratch using `Trigger`, `on_press`, `on_release`, `on_delta`
- **`addRule` / `clearRules`** — incremental and full replacement workflows
- **Direct mapper drive** — `onMouseButton`, `onMouseMove`, `onMouseScroll`, `onKey` bypassing the controller event path
- **`onTouchPan` / `onTouchPinch`** — touch gesture support
- **`resetState`** — clear held-button tracking on focus loss

## Key types

```
InputRule       — trigger + code + modifier_mask + on_press/release/delta
InputRule::Trigger — eMouseButton | eKey | eScroll | eTouchPan | eTouchPinch | eMouseDblClick
GestureAction   — eRotate | ePan | eZoom | eLook | eSetPivot
CameraActionType — semantic intent enum (eBeginRotate, ePanDelta, eZoomAtCursor, …)
CameraCommandPayload — x_px, y_px, delta_x_px, delta_y_px, zoom_factor, pressed
TouchPan / TouchPinch — touch gesture data structs
kModNone / kModShift / kModCtrl / kModAlt — modifier bitmask constants
```
