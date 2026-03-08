# Copilot PR Review Instructions

Use this for every PR review in this repository: optimize for correctness, safety, maintainability, testability, and project conventions.

## Review output format

- Start with a brief summary (1–3 bullets): what changed + biggest risks.
- Then list issues grouped by severity:
  - **BLOCKER** (must fix before merge)
  - **MAJOR** (strongly recommended)
  - **MINOR** (nice-to-have)
- For each issue: point to the exact file/area, explain *why*, and propose a concrete fix.
- If you're unsure, ask a clarifying question rather than guessing.

## Core C++ focus areas

### Correctness & API design

- Check invariants, pre/post-conditions, edge cases, and undefined behavior.
- Prefer clear ownership semantics (RAII, no raw owning pointers).
- Avoid surprising API changes; keep interfaces stable.
- Flag implicit conversions, narrowing, lifetime issues, dangling references, and iterator invalidation.

### Memory, lifetime, and RAII

- Look for leaks, double frees, use-after-free, and lifetime coupling bugs.
- Prefer `std::unique_ptr` for ownership and references/`std::span` for non-owning views.
- Ensure exception safety: strong/basic guarantees where appropriate.

### Performance (real-world)

- Flag accidental O(N²) behavior, unnecessary allocations/copies, and hidden sync points.
- Recommend `const&`, move semantics, reserve, and `std::span` when it helps.
- Prefer algorithmic improvements over micro-optimizations.
- Avoid premature "clever" tricks—only optimize with a clear benefit.

### Concurrency

- Identify data races, thread-unsafe singletons/statics, and unsafe shared mutation.
- Check lock ordering, deadlocks, and overly broad critical sections.
- Prefer immutable data or message-passing where possible.

### Error handling & logging

- Ensure failures are handled consistently (no silent ignores).
- Avoid throwing exceptions across module boundaries unless the project already standardizes it.
- Logs must be actionable; include context and avoid noisy logs in hot paths.

### Security / safety checks

- Flag unsafe parsing, unchecked inputs, path traversal, command injection patterns, and deserialization risks.
- Watch for integer overflow, buffer overrun, format-string issues, and risky `memcpy`/casts.

### Build system & portability

- Ensure changes compile on all supported compilers/OS (don't rely on non-standard extensions).
- Prefer standard C++ features and consistent warning levels.
- Avoid introducing platform-specific behavior without guards and tests.

### Documentation and readability

- Names should reveal intent; avoid abbreviations that hide meaning.
- Keep functions small; suggest refactors when complexity grows.
- Ensure public APIs have brief doc comments if behavior isn't obvious.

### CI expectations

- If the PR changes behavior, expect: tests updated/added.
- If the PR changes public API or core behavior, request a short PR description update with rationale and migration notes.

## Naming and style (compact)

Enforce the following; full rules and examples: [CODING_GUIDELINES.md](../CODING_GUIDELINES.md). Flag violations by citing the guideline and suggesting a concrete fix.

| Construct | Style | Example |
|-----------|-------|---------|
| Classes/Structs | PascalCase | `Buffer`, `ShaderCompiler` |
| Interface classes | `I` + PascalCase | `IRenderer`, `IBuffer` |
| Enums | PascalCase | `LogSink`, `ShaderStage` |
| Enum values | `e` + PascalCase + explicit value | `eNone = 0`, `eConsole = 1` |
| Functions/Methods | camelCase | `initialize()`, `createBuffer()` |
| Constants | `k` + PascalCase | `kMaxBufferSize` |
| Private/Protected members | snake_case + `_` | `buffer_size_`, `is_initialized_` |
| Macros | ALL_CAPS with VNE_ prefix | `VNE_ASSERT`, `VNE_PLATFORM_WINDOWS` |
| Namespaces | lowercase | `vne`, `xgl`, `xwin` |
| File names | snake_case | `shader_compiler.h` |
| Header guards | `#pragma once` | `#pragma once` |

## When reviewing test files

Apply this section when the PR changes files under `tests/` or `*_test.cpp`.

- New features/bug fixes should include tests unless clearly justified.
- Tests must be deterministic (no timeouts/flaky sleeps).
- Prefer clear Arrange-Act-Assert structure.
- If mocking: keep mocks minimal and verify behavior, not implementation details.
- Ensure tests run fast and don't require special hardware unless explicitly marked.

## This repository (VneTemplate)

### Project layout

- Library: **src/vertexnova/template/** — Public API: **include/vertexnova/template/** — Tests: **tests/** — Examples: **examples/**
- Dependencies: **deps/internal/** (e.g. vnecommon, vnelogging), **deps/external/** (e.g. googletest). No top-level `external/` or `libs/`.

### Formatting and static analysis

- **.clang-format** and **.clang-tidy** are enforced in separate CI actions; focus review on design and logic. Flag style/naming if automation might miss it.

### Extra review focus

- Flag unnecessary edits under **deps/internal/** or **deps/external/** unless the PR intentionally updates or configures submodules/dependencies.

Keep comments concrete: cite the guideline or rule and suggest a specific fix when possible.

## Checklist

- [ ] Summary (1–3 bullets) and issues grouped by BLOCKER / MAJOR / MINOR with file, reason, and fix.
- [ ] C++ focus areas considered (correctness, memory, perf, concurrency, errors, security, build, docs, CI).
- [ ] Naming/style and test rules (if PR touches tests) applied; violations cited with suggested fix.
- [ ] Repo layout and deps respected; no unnecessary deps edits.
- [ ] Concrete fixes suggested for each issue where applicable.
