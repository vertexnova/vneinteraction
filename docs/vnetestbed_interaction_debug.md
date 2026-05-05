# Debugging interaction direction in vnetestbed

If mouse look, pan, or movement feels **reversed** compared to unit tests in `vneinteraction`, the library math is usually correct; verify integration:

1. **Camera `GraphicsApi`** — Must match the renderer (e.g. OpenGL vs Metal/Vulkan). Mismatch breaks NDC Y handling in `mouseWindowToNDC` / pan deltas.
2. **Viewport vs window coordinates** — Multi-viewport or Dear ImGui overlays must pass **viewport-local** mouse coordinates to `Inspect3DController::onEvent` / `Navigation3DController::onEvent`. Feeding full-window coords while the mapper thinks the viewport is a sub-rectangle will invert or skew motion.
3. **Events reach the active controller** — Confirm `onResize(w,h)` matches the drawable region used for picking and projection.
4. **Quick instrumentation** — Log `delta_x_px` / `delta_y_px` on `eLookDelta` / `ePanDelta` and compare with expectations (same signs as `tests/navigation_3d_controller_test.cpp` and `tests/manipulator_regression_test.cpp`).

See also: `samples/glfw_opengl/03_test_interaction/demo_test_interaction.cpp` in the **vnetestbed** repo.
