# Third-person follow camera — future work (summary)

**Status:** Not implemented. This document is a short placeholder for a planned feature.

## Why

[`FollowBehavior`](../src/vertexnova/interaction/follow_behavior.cpp) / [`FollowController`](../include/vertexnova/interaction/follow_controller.h) provide **smooth tracking** of a world point with a fixed world-space offset and always **look-at** that point. They do **not** provide typical third-person game behavior: **orbit yaw/pitch**, **zoom by distance**, **shoulder offset**, or **optional camera–environment collision**.

## Intended behavior (high level)

- **Focus point:** target position + rotated target offset (e.g. chest/head).
- **Camera:** placed behind the focus along an orbit direction from **yaw/pitch** and **distance**; optional **shoulder** lateral offset.
- **Smoothing:** damp focus, distance, and camera position (exponential-style lerp).
- **Final pose:** `ICamera::lookAt(eye, focus, world_up)` (VertexNova cameras use look-at, not quaternion world rotation on the camera node).
- **Optional:** ray/sphere cast from focus toward camera to pull the eye in when blocked.

## Suggested placement

- New controller type in **vneinteraction** (e.g. `ThirdPersonFollowController`), parallel to `FollowController`.
- Camera instances remain **vnescene** [`ICamera`](../deps/internal/vnescene/include/vertexnova/scene/camera/camera.h).
- Optional **scene query** interface implemented by the host app (no physics engine inside the library).

## Phasing (when implemented)

1. **Core:** focus + orbit direction + distance + shoulder + smoothing + tests.
2. **Input:** explicit `addOrbit` / `addZoom` (and optionally `InputMapper` wiring).
3. **Collision:** minimal `ISceneQuery` + resolve step.
4. **Polish (optional):** yaw modes (e.g. align behind character), asymmetric collision smoothing, recenter, state-based offsets.

## Reference

Design discussion and full adaptation notes live in the Cursor plan **Third-person follow camera** (`third_person_follow_camera_*.plan.md` under `.cursor/plans/` if present).
