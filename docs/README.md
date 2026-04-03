# VneInteraction documentation

Human-written notes and Doxygen configuration for the **Vertexnova Interaction** library (camera manipulators, input mapping, and controllers for VertexNova).

## API reference (Doxygen)

From the repository root, enable Doxygen and build the doc target. HTML output is always under **the same directory you pass to `-B`**:

```bash
cmake -DENABLE_DOXYGEN=ON -B build
cmake --build build --target vneinteraction_doc_doxygen
```

Open `build/docs/html/index.html` in a browser.

`docs/doxyfile.in` sets `OUTPUT_DIRECTORY` to `<CMAKE_BINARY_DIR>/docs` (the main page is `html/index.html`). If you use another build tree (for example `build/shared`), open `<that-path>/docs/html/index.html` instead.

### Helper script

[`scripts/generate-docs.sh`](../scripts/generate-docs.sh) configures CMake with `ENABLE_DOXYGEN=ON` under **`build/shared`**, builds `vneinteraction_doc_doxygen`, then optionally validates Markdown links and prints a simple documentation-coverage summary.

```bash
./scripts/generate-docs.sh              # API docs, then link check + coverage hint
./scripts/generate-docs.sh --api-only   # CMake configure + Doxygen only
./scripts/generate-docs.sh --validate   # Link check + coverage only (no Doxygen build)
```

`doxygen` must be on `PATH`. Link validation runs only when `markdown-link-check` is installed.

### Requirements

- **Doxygen** — 1.9 or newer recommended.
- **Graphviz** (`dot`) — optional; CMake’s Doxygen `dot` component enables call graphs and related diagrams in the HTML output.

## Other documentation

- [Interaction module](vertexnova/interaction/interaction.md) — Architecture, intent model, usage, and figures (PNG exports from `vertexnova/interaction/diagrams/*.drawio`).
- [Library review & roadmap](vertexnova/tasks_plan/review_and_roadmap.md) — Comparison with common engines and medical viewers; prioritized enhancement notes.
- [Third-person follow camera (future)](vertexnova/tasks_plan/third_person_follow_camera_future.md) — Placeholder for planned third-person behavior beyond current `FollowController` / `FollowManipulator`.
