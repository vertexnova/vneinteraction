# VneInteraction Examples

This directory contains examples demonstrating the VneInteraction API.

## Building Examples

From the project root (use `build/shared` or `build/static`):

```bash
# Shared library build
cmake -B build/shared -DVNE_TEMPLATE_EXAMPLES=ON -DVNE_TEMPLATE_LIB_TYPE=shared
cmake --build build/shared

# Static library build
cmake -B build/static -DVNE_TEMPLATE_EXAMPLES=ON -DVNE_TEMPLATE_LIB_TYPE=static
cmake --build build/static
```

Alternatively, `-DVNE_TEMPLATE_DEV=ON` enables both tests and examples.

Executables are placed in `build/shared/bin/examples/` (or `build/static/bin/examples/`).

## Available Examples

### 01_hello_template — Getting Started

Minimal usage: call `vne::template_ns::hello()` and `get_version()`.

**Run:** `./build/<lib_type>/bin/examples/example_01_hello_template` (use `shared` or `static` to match your build)

## Quick Reference

| Example              | Focus          | Key Concepts              |
|----------------------|----------------|---------------------------|
| 01_hello_template    | Getting started| hello(), get_version()   |
