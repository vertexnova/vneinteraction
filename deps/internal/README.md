# Internal dependencies

VertexNova internal libraries go here. CMake modules (vnecmake) are in **cmake/vnecmake** at the project root.

- **vnecommon** – Common utilities and build helpers.  
  Clone or add as submodule into `deps/internal/vnecommon`.

- **vnelogging** – Logging library (e.g. spdlog-based).  
  Clone or add as submodule into `deps/internal/vnelogging`.

If these directories are missing or empty, the root CMake will skip them and the template library will build without linking to vne::common or vne::logging.

See each subdirectory’s README (once the repo is present) for recommended branch/tag and build options.
