# Vertexnova Interaction

## Overview

The **Vertexnova Interaction** library provides composable camera **manipulators**, **input mapping**, and **high-level controllers** for interactive 3D inspection, FPS-style navigation, 2D orthographic views, and follow cameras. It targets [vnescene](https://github.com/vertexnova/vnescene) cameras and [vneevents](https://github.com/vertexnova/vneevents) input types. It does **not** provide windowing, GL/Vulkan swapchains, or rendering.

**Characteristics:**

- **Event-driven** — Controllers accept `vne::events::Event` via `onEvent(event, delta_time)` and advance simulation-style state with `onUpdate(delta_time)`.
- **Intent layer** — `InputMapper` turns low-level events into semantic `CameraActionType` values and `CameraCommandPayload` data; manipulators consume only what they understand.
- **Composable rigs** — `CameraRig` holds zero or more `ICameraManipulator` instances and forwards every action and update to each (enables hybrid setups, e.g. orbit tooling beside free-look in an editor).
- **Focused math helpers** — Orbit and virtual-trackball geometry live in implementation-side helpers (`OrbitBehavior`, `TrackballBehavior` under `src/vertexnova/interaction/detail/`), composed by `OrbitalCameraManipulator`; they are not registered on the rig as standalone manipulators.

![System context](diagrams/context.png)

**Figure 1 — System context**

| Element | Description |
|---------|-------------|
| C++ Application | Your game, viewer, or editor: owns the window, feeds events, calls `setCamera` / `onResize` / `onEvent` / `onUpdate`. |
| vneinteraction | This library: controllers, `InputMapper`, `CameraRig`, manipulators. |
| vne::scene | Camera model: `ICamera`, perspective/orthographic types, `CameraFactory`. |
| vne::events | Input events: mouse, keyboard, scroll, touch. |
| vne::math | Vectors, quaternions, matrices (pulled in via scene and public headers). |
| vne::logging | Optional diagnostic logging; the library uses categorized log macros internally. |

**Diagram colors** — Draw.io sources in `diagrams/` use the [VertexNova visual style](https://learnvertexnova.com/docs/docs/misc/visual-style/) **primary palette** for slides and diagrams (canvas `#1C1C1E`, panels `#2C2C2E` / `#3A3A3C`, borders `#48484A`, accent orange `#E8622A` on tint `#2E1A07`, secondary stroke `#F28C5E`, text `#EBEBF0` / muted `#AEAEB2`).

If a PNG does not load, export the matching `.drawio` from [diagrams.net](https://app.diagrams.net) (same workflow as [vnelogging’s diagram export notes](../../../deps/internal/vnelogging/docs/vertexnova/logging/diagrams/README.md)).

![UML class relationships](diagrams/class.png)

**Figure 2 — Class diagram (major types and relationships)**

| Group | Types |
|-------|--------|
| Interfaces | `ICameraManipulator`, `ICameraController`, internal `IRotationStrategy` for orbit modes. |
| Base / rig / mapper | `CameraManipulatorBase`, `CameraRig`, `InputMapper`. |
| Manipulators | `OrbitalCameraManipulator`, `FreeLookManipulator`, `Ortho2DManipulator`, `FollowManipulator`. |
| Controllers | `Inspect3DController`, `Navigation3DController`, `Ortho2DController`, `FollowController`. |
| Orbit internals | `EulerOrbitStrategy`, `TrackballStrategy` (composition from `OrbitalCameraManipulator`). |

Export `diagrams/class.drawio` → `diagrams/class.png`.

![Runtime pipeline](diagrams/runtime-pipeline.png)

**Figure 3 — Runtime pipeline (per event and per frame)**

| Stage | Role |
|-------|------|
| Events | `vne::events::Event` delivered into the active controller. |
| Controller | Forwards to `InputMapper`; registers callback → `CameraRig::onAction`. |
| InputMapper | Emits `CameraActionType` + `CameraCommandPayload`. |
| CameraRig | Multicasts `onAction` / `onUpdate` to every manipulator. |
| Manipulators | Update pose, COI, zoom, or ortho extents. |
| Camera | `vne::scene::ICamera` stores the result for rendering and picking. |

The first page of `diagrams/runtime.drawio` (**1. Runtime pipeline**) is the source for this figure; export it to `diagrams/runtime-pipeline.png`. The same file’s other tabs (**2. Orbit rotate**, **3. FPS move + look**) are optional exports for deeper sequence-style views.

## Architecture

The design is **layered**: application and events sit above controllers; controllers own mapper + rig + manipulators; manipulators write to `ICamera`.

| Layer | Responsibility |
|-------|----------------|
| **Application** | Windowing, event pump, frame loop; calls controller API. |
| **Controller** | `ICameraController` implementations: translate events to mapper calls, wire mapper callbacks to rig, call `onUpdate` on the rig. |
| **Input mapping** | `InputMapper`: rules and presets map hardware events → `CameraActionType` + payload. |
| **Rig** | `CameraRig`: multicast `onAction` / `onUpdate` / lifecycle to all manipulators. |
| **Manipulator** | `ICameraManipulator`: orbit, free-look, ortho 2D, follow — each updates camera pose or parameters. |
| **Scene** | `vne::scene::ICamera`: authoritative camera state for the rest of the engine. |

![Component diagram](diagrams/component.png)

**Figure 4 — Public API vs implementation (layer view)**

| Swimlane | Contents |
|----------|----------|
| Public API | `interaction.h`, type headers (`interaction_types.h`, `camera_action.h`, `input_binding.h`, `camera_state.h`), controllers, `CameraRig`, `InputMapper`, manipulators, `ICameraManipulator` / `ICameraController`. |
| Implementation | `input_event_translator`, controller context helpers, rotation strategies (`IRotationStrategy`, Euler/trackball), `OrbitBehavior` / `TrackballBehavior`, `camera_math` / `view_math`, and per-class `.cpp` files. |

Export `diagrams/component.drawio` → `diagrams/component.png`.

## Intent model (events → actions)

- **`InputMapper`** — Built from `InputRule` rows (keys, mouse buttons, modifiers, gestures). **Presets** include `orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, and `orthoPreset`.
- **`CameraActionType`** — Semantic commands such as `eBeginRotate`, `eRotateDelta`, `eEndRotate`, pan and zoom actions, free-look deltas, WASD moves, modifiers, reset, pivot-at-cursor, and optional discrete speed keys (declared in `camera_action.h`).
- **`CameraCommandPayload`** — Cursor position, deltas, zoom factor, and button/pressed flags carried with actions.
- **`GestureAction`** — High-level gesture identifiers used with remapping helpers (`bindGesture`, scroll, double-click bindings) without exposing full `InputRule` details to callers.

Manipulators **ignore** actions they do not implement; the rig does not filter per manipulator.

## Key components

### `ICameraManipulator` and `CameraManipulatorBase`

**`ICameraManipulator`** (`camera_manipulator.h`) — Core manipulator contract:

- `onAction`, `onUpdate`, `setCamera`, `onResize`, `resetState`, `isEnabled`, `setEnabled`.

**`CameraManipulatorBase`** (`camera_manipulator_base.h`) — Shared zoom dispatch (`ZoomMethod`: dolly, scene scale, FOV) and common camera/viewport helpers for concrete manipulators.

### Manipulators

#### `OrbitalCameraManipulator`

Orbit around a center of interest: **Euler** or **trackball** rotation (`OrbitRotationMode`), pivot modes (`OrbitPivotMode`), pan, zoom-to-cursor / dolly / FOV, inertia, `fitToAABB`. Delegates rotation math to internal **Euler** / **trackball** strategies and behavior helpers.

#### `FreeLookManipulator`

**FPS** or **Fly** mode: WASD-style motion, mouse look, sprint/slow modifiers; works with perspective or orthographic cameras (ortho uses in-plane pan semantics where applicable).

#### `Ortho2DManipulator`

**Orthographic** cameras only: pan, zoom-at-cursor, optional in-plane rotation, inertia.

#### `FollowManipulator`

Smooth follow of a world target or a callback-provided target; configurable offset and damping.

### Controllers (`ICameraController`)

| Class | Role |
|-------|------|
| `Inspect3DController` | 3D inspection (medical, CAD-style): `OrbitalCameraManipulator` + `InputMapper` (orbit preset), pivot and DOF toggles, `fitToAABB`. |
| `Navigation3DController` | World traversal: `FreeLookManipulator` + `InputMapper` (FPS-style preset), mode and binding configuration. |
| `Ortho2DController` | 2D ortho viewports: `Ortho2DManipulator` + ortho preset. |
| `FollowController` | Follow camera: `FollowManipulator` only; no user input mapping required. |

### Input and rig

#### `InputMapper`

Maps mouse, keyboard, scroll, and touch-style input to **callbacks** invoking `(CameraActionType, CameraCommandPayload, double dt)`. Supports adding rules and replacing the full rule set (used when controllers rebuild bindings after mode or DOF changes).

#### `CameraRig`

- **Lifecycle** — `setCamera`, `onResize`, `resetState`.
- **Dispatch** — `onAction`, `onUpdate` to every registered manipulator.
- **Factories** — `makeOrbit()`, `makeTrackball()`, `makeFps()`, `makeFly()`, `makeOrtho2D()`, `makeFollow()` build rigs with a single default manipulator; you can still `addManipulator` for custom stacks.

### Shared headers and types

| Header | Role |
|--------|------|
| `interaction.h` | Umbrella include for full API surface (manipulators, rig, mapper, controllers, types). |
| `interaction_types.h` | Aggregates behavioral enums and re-exports `camera_action`, `camera_state`, `input_binding`. |
| `camera_action.h` | `CameraActionType`, `CameraCommandPayload`, `GestureAction`. |
| `camera_state.h` | Grouped state structs for orbit, trackball, free-look, etc. |
| `input_binding.h` | `InputRule`, mouse/key bindings, touch helpers, modifier constants. |
| `version.h` | `get_version()` string. |

### Implementation layout (`src/vertexnova/interaction/`)

One **`.cpp` per public class** where applicable, plus `input_mapper.cpp`, `camera_rig.cpp`, `camera_manipulator_base.cpp`, `input_event_translator.cpp`, `camera_math.cpp`, `version.cpp`, and **`detail/`** sources for orbit/trackball behaviors and rotation strategies.

## Usage examples

The examples below progress from the simplest one-controller setup to advanced composition patterns.
All snippets assume:

```cpp
#include <vertexnova/interaction/interaction.h>
// camera factory lives in:
#include <vertexnova/scene/camera/camera.h>
```

### Use-case quick reference

| Use case | Recommended controller | Default bindings |
|---|---|---|
| 3D model viewer / CAD / medical | `Inspect3DController` | LMB=orbit, RMB/MMB=pan, scroll=zoom, double-click LMB=set pivot |
| FPS walkthrough / game editor | `Navigation3DController` (eFps) | WASD+mouse look, RMB=look, scroll=zoom, Shift=sprint, Ctrl=slow |
| Flight / space / drone sim | `Navigation3DController` (eFly) | Same as FPS, pitch unconstrained |
| 2D slice / map / diagram | `Ortho2DController` | RMB/MMB=pan, scroll=zoom |
| Cinematic follow / third-person | `FollowController` | No user input; target-driven |
| Hybrid editor (orbit + walk) | `Inspect3DController` + `Navigation3DController` | Hot-key toggle; both share one `ICamera` |
| Custom rig (advanced) | `CameraRig` directly | Compose any manipulators; wire your own `InputMapper` |

---

### 1. `Inspect3DController` — 3D orbit inspection

The most common setup: a perspective camera orbiting a center of interest (COI).

```cpp
#include <vertexnova/interaction/interaction.h>
#include <vertexnova/scene/camera/camera.h>

// ── Camera setup ───────────────────────────────────────────────────────────
auto camera = vne::scene::CameraFactory::createPerspective(
    vne::scene::PerspectiveCameraParameters(
        /*fov_y_deg=*/60.0f, /*aspect=*/16.0f / 9.0f,
        /*near=*/0.1f, /*far=*/1000.0f));

camera->setPosition(vne::math::Vec3f(4.0f, 3.0f, 6.0f));
camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f),   // look-at target (COI)
               vne::math::Vec3f(0.0f, 1.0f, 0.0f));   // world up

// ── Controller setup ───────────────────────────────────────────────────────
vne::interaction::Inspect3DController ctrl;
ctrl.setCamera(camera);
ctrl.onResize(1280.0f, 720.0f);   // must call once before first event/update

// Tune sensitivity (optional — defaults are production-ready)
ctrl.setRotateSensitivity(0.4f);   // degrees per pixel
ctrl.setPanSensitivity(1.0f);      // pan scale multiplier
ctrl.setZoomSensitivity(1.2f);     // pow(factor, zoom_sensitivity) per scroll tick

// Enable/disable degrees-of-freedom
ctrl.setRotationEnabled(true);
ctrl.setPanEnabled(true);
ctrl.setZoomEnabled(true);

// ── Game / render loop ─────────────────────────────────────────────────────
// Feed every platform event and call onUpdate each frame.
// dt is seconds since last frame (e.g. 0.016 ≈ 60 fps).
while (!window.shouldClose()) {
    double dt = timer.tick();
    for (auto& event : window.pollEvents())
        ctrl.onEvent(event, dt);
    ctrl.onUpdate(dt);

    renderer.drawFrame(camera);
}
```

**Fit to content** — snap the camera so the object fills the view:

```cpp
// Call after loading a mesh or whenever the scene bounds change.
vne::math::Vec3f aabb_min(-1.0f, -1.0f, -1.0f);
vne::math::Vec3f aabb_max( 1.0f,  1.0f,  1.0f);
ctrl.fitToAABB(aabb_min, aabb_max);
```

**View direction presets** — orient to a canonical face (useful for toolbar buttons):

```cpp
ctrl.orbitalCameraManipulator()->setViewDirection(
    vne::interaction::ViewDirection::eTop);    // or eFront, eRight, eIso, …
```

---

### 2. `Inspect3DController` — Trackball rotation mode

Trackball maps mouse position to a virtual sphere for physically intuitive, orientation-preserving rotation. Switch at any time; the controller re-syncs from the live camera.

```cpp
vne::interaction::Inspect3DController ctrl;
ctrl.setCamera(camera);
ctrl.onResize(width, height);

// Switch to quaternion trackball rotation
ctrl.setRotationMode(vne::interaction::OrbitalRotationMode::eTrackball);

// Optional: choose projection model
//   eHyperbolic — smooth centre-to-rim transition (default)
//   eRim        — pure rim mapping, more aggressive spin at edges
ctrl.orbitalCameraManipulator()->setTrackballProjectionMode(
    vne::interaction::TrackballProjectionMode::eHyperbolic);

// Scale the rotation response (default 2.5 — larger = faster spin)
ctrl.orbitalCameraManipulator()->setTrackballRotationScale(2.0f);
```

---

### 3. `Inspect3DController` — Pivot modes

Pivot mode controls the center of interest that the orbit rotates around.

```cpp
auto* manip = ctrl.orbitalCameraManipulator();

// ── (a) Follow center of interest (default) ───────────────────────────────
// COI moves with pans; orbit radius stays constant.
manip->setPivotMode(vne::interaction::OrbitPivotMode::eCoi);

// ── (b) Always orbit the view-space center ────────────────────────────────
// COI is re-derived from the camera each frame (good for unconstrained fly).
manip->setPivotMode(vne::interaction::OrbitPivotMode::eViewCenter);

// ── (c) Fixed world-space landmark ────────────────────────────────────────
// Useful for medical viewers: lock the orbit to an anatomical landmark.
manip->setPivotMode(vne::interaction::OrbitPivotMode::eFixed);
manip->setPivot(vne::math::Vec3f(0.5f, 1.2f, -0.3f),
                vne::interaction::CenterOfInterestSpace::eWorldSpace);

// Enable double-click-to-pivot (re-projects cursor to world and sets COI)
ctrl.setPivotOnDoubleClickEnabled(true);
```

---

### 4. `Navigation3DController` — FPS and Fly modes

First-person traversal with WASD movement and mouse look. Two sub-modes:

```cpp
vne::interaction::Navigation3DController nav;
nav.setCamera(camera);
nav.onResize(width, height);

// ── FPS mode (default) — world-up constrained ─────────────────────────────
nav.setMode(vne::interaction::FreeLookMode::eFps);
nav.setMoveSpeed(5.0f);            // world units per second
nav.setMouseSensitivity(0.15f);    // degrees per pixel
nav.setSprintMultiplier(3.0f);     // speed × 3 while Shift held
nav.setSlowMultiplier(0.2f);       // speed × 0.2 while Ctrl held

// ── Fly mode — unconstrained 6DoF (good for space / underwater scenes) ────
nav.setMode(vne::interaction::FreeLookMode::eFly);

// ── Loop (same as Inspect3D) ──────────────────────────────────────────────
while (!window.shouldClose()) {
    double dt = timer.tick();
    for (auto& event : window.pollEvents())
        nav.onEvent(event, dt);
    nav.onUpdate(dt);   // integrates held-key motion each frame

    renderer.drawFrame(camera);
}
```

**Rebind keys** (e.g. game-pad axes or a non-WASD layout):

```cpp
// All key codes are vne::events::KeyCode values (mirrors GLFW).
nav.setMoveForwardKey(vne::events::KeyCode::eUp);
nav.setMoveBackwardKey(vne::events::KeyCode::eDown);
nav.setMoveLeftKey(vne::events::KeyCode::eLeft);
nav.setMoveRightKey(vne::events::KeyCode::eRight);
nav.setSpeedBoostKey(vne::events::KeyCode::eLeftShift);
nav.setLookButton(vne::events::MouseButton::eRight);   // hold RMB to look
```

**Discrete speed steps** — let users adjust speed in-flight with `[` and `]`:

```cpp
nav.setIncreaseMoveSpeedKey(vne::events::KeyCode::eRightBracket);  // ]
nav.setDecreaseMoveSpeedKey(vne::events::KeyCode::eLeftBracket);   // [
nav.setMoveSpeedStep(1.5f);   // multiplicative step
nav.setMoveSpeedMin(0.1f);
nav.setMoveSpeedMax(200.0f);
```

---

### 5. `Ortho2DController` — 2D slice / map view

Requires an orthographic camera. Supports pan, zoom-at-cursor, and optional in-plane rotation.

```cpp
// Create an orthographic camera
auto ortho_cam = vne::scene::CameraFactory::createOrthographic(
    vne::scene::OrthoCameraParameters(
        /*width=*/10.0f, /*height=*/7.5f,
        /*near=*/-100.0f, /*far=*/100.0f));

vne::interaction::Ortho2DController ortho;
ortho.setCamera(ortho_cam);
ortho.onResize(width, height);

// Enable in-plane rotation (disabled by default — most slice viewers
// want pure pan/zoom without accidental rotation).
ortho.setRotationEnabled(false);
ortho.setPanEnabled(true);
ortho.setZoomEnabled(true);

// Sensitivity tweaks
ortho.setPanSensitivity(1.0f);
ortho.setZoomSensitivity(1.15f);

// Fit a 2D region to the viewport (e.g. after loading a DICOM slice)
ortho.fitToAABB(vne::math::Vec3f(-256.0f, -256.0f, -1.0f),
                vne::math::Vec3f( 256.0f,  256.0f,  1.0f));

while (!window.shouldClose()) {
    double dt = timer.tick();
    for (auto& event : window.pollEvents())
        ortho.onEvent(event, dt);
    ortho.onUpdate(dt);
    renderer.drawFrame(ortho_cam);
}
```

---

### 6. `FollowController` — smooth follow camera

The follow controller tracks a moving target each frame. No user rotation/pan input is needed — the camera simply chases the target.

```cpp
vne::interaction::FollowController follow;
follow.setCamera(camera);
follow.onResize(width, height);

// Fixed offset from the target in world space (behind and above)
follow.setOffset(vne::math::Vec3f(0.0f, 3.0f, -8.0f));

// Lag [0, 1]: 0 = instant snap, 1 = never moves. ~0.1 gives a cinematic feel.
follow.setLag(0.08f);

// ── Option A: static target transform ─────────────────────────────────────
vne::math::Mat4f target_transform = /* scene node world matrix */;
follow.setTarget(target_transform);

// ── Option B: dynamic callback — called every onUpdate() ──────────────────
follow.setTarget([&]() -> vne::math::Mat4f {
    return scene.getNode("robot_head")->worldTransform();
});

while (!window.shouldClose()) {
    double dt = timer.tick();
    // No events needed unless you want zoom or override from the follow
    follow.onUpdate(dt);    // advances smooth-follow integration
    renderer.drawFrame(camera);
}
```

---

### 7. Runtime controller switching

A common editor pattern: an orbit view for inspection and a fly view for scene traversal, both driving the same camera.

```cpp
enum class ActiveView { eInspect, eNavigate };
ActiveView active = ActiveView::eInspect;

vne::interaction::Inspect3DController inspect;
vne::interaction::Navigation3DController nav;

auto camera = vne::scene::CameraFactory::createPerspective(params);
inspect.setCamera(camera);
inspect.onResize(width, height);
nav.setCamera(camera);
nav.onResize(width, height);

auto activeCtrl = [&]() -> vne::interaction::ICameraController* {
    return active == ActiveView::eInspect
        ? static_cast<vne::interaction::ICameraController*>(&inspect)
        : static_cast<vne::interaction::ICameraController*>(&nav);
};

while (!window.shouldClose()) {
    double dt = timer.tick();

    for (auto& event : window.pollEvents()) {
        // Toggle on Tab key
        if (auto* ke = dynamic_cast<vne::events::KeyPressedEvent*>(&event)) {
            if (ke->getKeyCode() == vne::events::KeyCode::eTab) {
                active = (active == ActiveView::eInspect)
                    ? ActiveView::eNavigate : ActiveView::eInspect;
                continue;
            }
        }
        activeCtrl()->onEvent(event, dt);
    }
    activeCtrl()->onUpdate(dt);
    renderer.drawFrame(camera);
}
```

> Both controllers stay alive across the switch; the inactive one simply stops receiving events. No camera state is lost.

---

### 8. Custom input bindings via `InputMapper`

The high-level controllers expose their mapper via `inputMapper()`. You can replace or extend the rule set for application-specific workflows (e.g. a CAD-style MMB orbit, or a pen-tablet workflow with no mouse buttons).

```cpp
// ── Replace with a CAD-style binding ─────────────────────────────────────
ctrl.inputMapper().setRules(vne::interaction::InputMapper::cadPreset());

// ── Selectively rebind a single gesture ──────────────────────────────────
// Swap rotate from LMB to MMB, keep everything else.
ctrl.setRotateButton(vne::events::MouseButton::eMiddle);
ctrl.setPanButton(vne::events::MouseButton::eRight);

// ── Add a custom rule on top of an existing preset ────────────────────────
// Ctrl+Scroll = discrete step zoom (instead of continuous).
ctrl.inputMapper().bindScroll(
    vne::interaction::GestureAction::eZoom,
    vne::interaction::InputMapper::kModCtrl);

// ── Disable zoom entirely at runtime ─────────────────────────────────────
ctrl.setZoomEnabled(false);     // suppresses zoom rules in the mapper

// ── Clear all rules and start from scratch ───────────────────────────────
ctrl.inputMapper().clearRules();
ctrl.inputMapper().addRule(/* custom InputRule ... */);
```

---

### 9. Building a custom `CameraRig`

Use `CameraRig` directly when you need a hybrid or non-standard manipulator stack without a high-level controller façade.

```cpp
#include <vertexnova/interaction/camera_rig.h>
#include <vertexnova/interaction/input_mapper.h>
#include <vertexnova/interaction/orbital_camera_manipulator.h>
#include <vertexnova/interaction/free_look_manipulator.h>

// ── Build the rig ─────────────────────────────────────────────────────────
auto rig = std::make_unique<vne::interaction::CameraRig>();
rig->setCamera(camera);
rig->onResize(width, height);

// makeOrbit() returns OrbitalCameraManipulator* (owned by the rig)
auto* orbit = rig->makeOrbit();
orbit->setPivotMode(vne::interaction::OrbitPivotMode::eCoi);
orbit->setRotationInertiaEnabled(true);
orbit->setRotationDamping(4.0f);   // higher = faster stop

// Add a secondary FreeLookManipulator for WASD while orbiting
auto* fly = rig->makeFly();
fly->setMoveSpeed(2.0f);
fly->setHandleZoom(false);   // let the orbit manipulator own zoom

// ── Wire InputMapper ──────────────────────────────────────────────────────
vne::interaction::InputMapper mapper;
mapper.setRules(vne::interaction::InputMapper::gamePreset());
mapper.setActionCallback([&](vne::interaction::CameraActionType action,
                              const vne::interaction::CameraCommandPayload& payload,
                              double dt) {
    rig->onAction(action, payload, dt);
});

// ── Frame loop ────────────────────────────────────────────────────────────
while (!window.shouldClose()) {
    double dt = timer.tick();
    for (auto& event : window.pollEvents())
        translateEventToMapper(event, mapper, dt);   // your event→mapper bridge
    rig->onUpdate(dt);
    renderer.drawFrame(camera);
}
```

---

### 10. Bridging platform events

Controllers and the `InputMapper` accept `vne::events::Event` objects. Forward them from your platform (GLFW, SDL, Win32, custom) using `input_event_translator` conventions. The minimum event types required per controller:

| Controller | Required event types |
|------------|---------------------|
| `Inspect3DController` | `MouseButtonPressed/Released`, `MouseMoved`, `MouseScrolled` |
| `Navigation3DController` | `MouseButtonPressed/Released`, `MouseMoved`, `MouseScrolled`, `KeyPressed/Released` |
| `Ortho2DController` | `MouseButtonPressed/Released`, `MouseMoved`, `MouseScrolled` |
| `FollowController` | None (driven by `onUpdate` only) |

A typical GLFW bridge:

```cpp
// Called from your GLFW mouse-button callback:
void onMouseButton(GLFWwindow*, int btn, int action, int /*mods*/) {
    auto vne_btn = static_cast<vne::events::MouseButton>(btn);  // 0/1/2 → Left/Right/Middle
    if (action == GLFW_PRESS)
        activeCtrl()->onEvent(vne::events::MouseButtonPressedEvent(vne_btn, cursor_x, cursor_y), dt);
    else
        activeCtrl()->onEvent(vne::events::MouseButtonReleasedEvent(vne_btn, cursor_x, cursor_y), dt);
}

// Called from your GLFW cursor-position callback:
void onCursorPos(GLFWwindow*, double x, double y) {
    activeCtrl()->onEvent(vne::events::MouseMovedEvent(x, y), dt);
}

// Called from your GLFW scroll callback:
void onScroll(GLFWwindow*, double /*dx*/, double dy) {
    activeCtrl()->onEvent(vne::events::MouseScrolledEvent(0.0, dy), dt);
}

// Called from your GLFW key callback:
void onKey(GLFWwindow*, int key, int /*scan*/, int action, int /*mods*/) {
    auto vne_key = static_cast<vne::events::KeyCode>(key);
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        activeCtrl()->onEvent(vne::events::KeyPressedEvent(vne_key), dt);
    else if (action == GLFW_RELEASE)
        activeCtrl()->onEvent(vne::events::KeyReleasedEvent(vne_key), dt);
}

// Per-frame (inside your render loop):
void onUpdate(double dt) {
    activeCtrl()->onUpdate(dt);
}
```

> See the `03_test_interaction` sample for a complete working GLFW integration (`samples/glfw_opengl/03_test_interaction/demo_test_interaction.cpp`).

## Integration with other VertexNova modules

| Module | How interaction uses it |
|--------|-------------------------|
| **vne::scene** | `ICamera` and concrete camera types; pose and projection writes. |
| **vne::events** | Event types and enums for input. |
| **vne::math** | Linear algebra and utilities (via scene and headers). |
| **vne::logging** | Internal diagnostics; configure logging in the host app if you want sink output from this library (patterns mirror [vnelogging](../../../deps/internal/vnelogging/docs/vertexnova/logging/logging.md)). |

## Build configuration (CMake)

| Option | Default | Description |
|--------|---------|-------------|
| `VNE_INTERACTION_TESTS` | ON | Build unit tests. |
| `VNE_INTERACTION_EXAMPLES` | OFF | Build example programs. |
| `VNE_INTERACTION_DEV` | ON at repo root | Dev preset: tests and examples enabled. |
| `VNE_INTERACTION_CI` | OFF | CI preset: tests ON, examples OFF. |
| `VNE_INTERACTION_LIB_TYPE` | `shared` | `static` or `shared`. |
| `ENABLE_DOXYGEN` | OFF | Generate Doxygen HTML API docs. |

### Static vs shared

- **`static`** (`-DVNE_INTERACTION_LIB_TYPE=static`) — Single binary, no separate interaction DLL/dylib to ship.
- **`shared`** (default) — Suitable for plugins or multiple executables sharing one build; on Windows use `VNE_INTERACTION_API` for correct export/import.

## API documentation (Doxygen)

Enable API docs the same way as in **vnelogging**:

```bash
cmake -DENABLE_DOXYGEN=ON -B build
cmake --build build --target vneinteraction_doc_doxygen
```

HTML output is written under `build/docs/html/` (see `docs/doxyfile.in` `OUTPUT_DIRECTORY`).

## Testing

The repository includes GoogleTest-based tests (manipulators, mappers, rigs, controllers, regression cases). After configuring CMake with tests enabled, build and run the test binary produced for `vneinteraction_tests` (for example from your build tree: `bin/vneinteraction_tests` or via `ctest`).

## Requirements

- **C++20** or higher (as set by this project’s CMake).
- **CMake** 3.19+.
- **Dependencies** — **vnescene**, **vneevents**, and transitive **vnemath** / **vne::logging** / **vne::common** as declared by this repo’s CMake (`deps/internal`).
