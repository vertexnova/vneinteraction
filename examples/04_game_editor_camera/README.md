# 04 — Game / Editor Camera

Full coverage of `Navigation3DController` and `FreeLookManipulator`.

## Use case

FPS games, game editor viewports, flight sims, architectural walkthroughs, drone simulators — any scene where the user physically moves through the world.

## What it covers

### A — FPS mode, WASD + mouse look
- `setMode(FreeLookMode::eFps)` — world-up fixed, pitch clamped ±89°
- `setMoveSpeed`, `setMouseSensitivity`, `setSprintMultiplier`, `setSlowMultiplier`
- Read back with `getMoveSpeed`, `getMouseSensitivity`, `getSprintMultiplier`, `getSlowMultiplier`
- RMB hold → look, W/A/S/D → move, E → move up, Q → move down

### B — Sprint and slow modifiers
- Shift held + W → sprint (speed × `sprintMultiplier`)
- Ctrl held + W → slow-walk (speed × `slowMultiplier`)

### C — Fly mode
- `setMode(FreeLookMode::eFly)` — unconstrained, pitch can exceed ±89°, allows barrel roll
- Useful for space sims, underwater scenes, drone simulators

### D — Key rebinding (arrow key layout)
- `setMoveForwardKey(eUp)` / `setMoveBackwardKey(eDown)` / `setMoveLeftKey(eLeft)` / `setMoveRightKey(eRight)`
- `setMoveUpKey(ePageUp)` / `setMoveDownKey(ePageDown)` — full 6-DoF bindings
- `setSpeedBoostKey(eRightShift)` — single key replaces default Left/Right Shift pair
- `setLookButton(eLeft)` — reassign look from RMB to LMB

### E — DOF gating
- `setMoveEnabled(false)` / `isMoveEnabled()` — WASD ignored
- `setLookEnabled(false)` / `isLookEnabled()` — mouse delta ignored
- `setZoomEnabled(false)` / `isZoomEnabled()` — scroll ignored

### F — Discrete speed-step keys
- `setIncreaseMoveSpeedKey(eRightBracket)` / `setDecreaseMoveSpeedKey(eLeftBracket)`
- `setMoveSpeedStep(2.0f)` — multiplicative factor per press
- `setMoveSpeedMin` / `setMoveSpeedMax` — clamp range
- Read back `getMoveSpeed()` after presses to verify

### G — `freeLookManipulator()` escape hatch
- `setWorldUp(Z-up)` — change world convention for CAD / scientific apps
- `setHandleZoom(false)` — let another rig member own scroll zoom
- `markAnglesDirty()` — force re-sync of yaw/pitch from camera pose after external move

### H — ZoomMethod variants
- `setZoomMethod(eSceneScale / eChangeFov / eDollyToCoi)` via escape hatch

### I — fitToAABB + reset()
