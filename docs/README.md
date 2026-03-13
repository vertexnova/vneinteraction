# VneInteraction Documentation

Documentation for the VneInteraction library (camera behaviors and controllers for VertexNova).

## Generating API Documentation

Generate API documentation with Doxygen:

```bash
cmake -DENABLE_DOXYGEN=ON -B build
cmake --build build --target vneinteraction_doc_doxygen
```

Documentation will be available at `build/docs/html/index.html` (or `build/shared/docs/html/index.html` if using a named build directory).

Alternatively, use the script (if available):

```bash
./scripts/generate-docs.sh --api-only
```

## Requirements

- Doxygen 1.8.13 or higher
- Graphviz (optional; for class and call graphs)

## Other documentation

- [Architecture & design](vertexnova/interaction/interaction.md) — Module overview, components, and usage.
