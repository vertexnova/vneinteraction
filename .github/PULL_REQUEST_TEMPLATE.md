## Description

<!-- Brief description of the change -->

**Release notes:** Use a [Conventional Commits](https://www.conventionalcommits.org/)–style PR title (e.g. `feat: add X`, `fix: resolve Y`, `docs: update Z`) so [release-please](https://github.com/googleapis/release-please) can include this change in the changelog. If you squash-merge, use the PR title as the commit message.

## Checklist

- [ ] Project builds (e.g. `cmake -B build/shared` and `cmake --build build/shared`, or platform script with `-l shared`/`-l static`).
- [ ] Tests pass (e.g. `ctest --test-dir build/shared` or script `-a test`).
- [ ] Code is formatted (e.g. run `clang-format` as configured for this repo); CI clang-format will check.
- [ ] Docs updated if you changed behavior or public API.

## Additional notes

<!-- Optional: migration notes, follow-up work, etc. -->
