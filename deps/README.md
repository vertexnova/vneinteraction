# Dependencies

This directory holds external and internal dependencies for VneInteraction.

## Layout

- **external/** – Third-party dependencies (e.g. Google Test).
- **internal/** – VertexNova internal libraries (vnecommon, vnelogging, vnemath, vnescene).

CMake modules (vnecmake) live in **cmake/vnecmake** at the project root; see main [README](../README.md).

## Getting dependencies

VneInteraction’s `.gitmodules` defines these submodules:

| Submodule | Path | URL |
|-----------|------|-----|
| vnecmake | `cmake/vnecmake` | https://github.com/vertexnova/vnecmake.git |
| googletest | `deps/external/googletest` | https://github.com/google/googletest.git |
| vnecommon | `deps/internal/vnecommon` | https://github.com/vertexnova/vnecommon.git |
| vnelogging | `deps/internal/vnelogging` | https://github.com/vertexnova/vnelogging.git |
| vnemath | `deps/internal/vnemath` | https://github.com/vertexnova/vnemath.git |
| vnescene | `deps/internal/vnescene` | https://github.com/vertexnova/vnescene.git |

After cloning, initialize and update them from the project root:

```bash
git submodule update --init --recursive
```

The sections below give per-dependency details and optional `git submodule add` commands for repos that don’t yet have these entries.

### vnecmake (required)

CMake modules come from the **vnecmake** submodule at `cmake/vnecmake`. From the project root:

```bash
git submodule add <vnecmake-repo-url> cmake/vnecmake
git submodule update --init --recursive
```

If already in `.gitmodules`, run `git submodule update --init --recursive`.

### External: Google Test

Tests use Google Test from `deps/external/googletest`. Either:

1. **Git submodule (recommended)**  
   From the project root:
   ```bash
   git submodule add https://github.com/google/googletest.git deps/external/googletest
   git submodule update --init --recursive
   ```
   Use tag `v1.17.0` (or later) if you pin:  
   `cd deps/external/googletest && git checkout v1.17.0`

2. **FetchContent fallback**  
   If `deps/external/googletest` is not present, the CMake configuration for tests will use FetchContent to download googletest (v1.17.0) at configure time.

### Internal: vnecommon, vnelogging, vnemath, vnescene

The library links to VertexNova internal dependencies when present:

- **deps/internal/vnecommon** – Common utilities.
- **deps/internal/vnelogging** – Logging (e.g. spdlog-based).
- **deps/internal/vnemath** – Math (vectors, quaternions, matrices); required by vnescene.
- **deps/internal/vnescene** – Cameras and scene (ICamera, PerspectiveCamera, etc.); required.

**In this repo (VneInteraction):** `.gitmodules` already lists these paths. Just run from the project root:

```bash
git submodule update --init --recursive
```

**In a downstream repo** that does not yet have these submodules in `.gitmodules`, add them from the project root (omit any that already exist):

```bash
git submodule add https://github.com/vertexnova/vnecommon.git deps/internal/vnecommon
git submodule add https://github.com/vertexnova/vnelogging.git deps/internal/vnelogging
git submodule update --init --recursive
```

See the README in each internal dependency directory for recommended branch/tag and build options.
