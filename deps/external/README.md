# External dependencies

Third-party dependencies live here.

## googletest

- **Path:** `googletest/`
- **Purpose:** Unit testing (gtest, gmock).
- **Recommended version:** v1.17.0 or later.

Add as a submodule from the project root:

```bash
git submodule add https://github.com/google/googletest.git deps/external/googletest
cd deps/external/googletest && git checkout v1.17.0
```

If this directory is empty, the build will use CMake FetchContent to download googletest when tests are enabled.
