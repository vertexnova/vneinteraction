# VneInteraction scripts

Helper scripts for building, formatting, and documentation. Run them from the **repository root** (paths below assume `./scripts/...`).

## Build scripts

Platform scripts place the CMake binary directory under `build/<lib_type>/<build_type>/…` (default `lib_type` is **shared**). Example binaries end up in `<build-dir>/bin/examples/`.

### Linux (`build_linux.sh`)

```bash
./scripts/build_linux.sh -t Debug -a configure_and_build
./scripts/build_linux.sh -c clang -j 20 -t Release
./scripts/build_linux.sh -l static -t Release -clean
./scripts/build_linux.sh -interactive
```

**Options:** `-t` build type (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`), `-a` action (`configure` \| `build` \| `configure_and_build` \| `test`), `-c` compiler (`gcc` \| `clang`), `-l` library type (`static` \| `shared`), `-j` jobs, `-clean`, `-interactive`, `-h`

### macOS (`build_macos.sh`)

```bash
./scripts/build_macos.sh -t Debug -a configure_and_build
./scripts/build_macos.sh -xcode -t Release
./scripts/build_macos.sh -xcode-only -t Release
./scripts/build_macos.sh -a xcode_build -t Debug
./scripts/build_macos.sh -interactive
```

**Options:** `-t` build type, `-a` action (`configure` \| `build` \| `configure_and_build` \| `test` \| `xcode` \| `xcode_build`), `-l` (`static` \| `shared`), `-xcode` (also generate Xcode project), `-xcode-only`, `-j` jobs, `-clean`, `-interactive`, `-h`

### Windows

**Bash** (Git Bash / WSL): `build_windows.sh` — requires `cl` and CMake on `PATH` (Visual Studio Developer environment).

```bash
./scripts/build_windows.sh -t Debug -a configure_and_build
./scripts/build_windows.sh -j 8 -t Release -l static
```

**Options:** `-t` / `--build-type`, `-a` / `--action`, `-l` / `--lib-type`, `-j` / `--jobs`, `-clean` / `--clean`, `--interactive`, `-h` / `--help`

**Python** (native Windows): `build_windows.py` — run from a **Visual Studio Developer Command Prompt**.

```bash
python scripts/build_windows.py -t Debug -a configure_and_build
python scripts/build_windows.py --build-type Release --jobs 8
python scripts/build_windows.py --interactive
python scripts/build_windows.py --build-type Release --clean
```

**PowerShell** (`build_windows.ps1`): same actions and defaults as the Bash script; parameters `-BuildType`, `-LibType`, `-Action`, `-Clean`, `-Jobs`.

```powershell
.\scripts\build_windows.ps1 -BuildType Release -Action test
```

## Documentation (`generate-docs.sh`)

Doxygen via CMake (default tree `build/shared`). See [docs/README.md](../docs/README.md) for details.

```bash
./scripts/generate-docs.sh
./scripts/generate-docs.sh --api-only
./scripts/generate-docs.sh --validate
```

## Formatting (`clang_formatter.py`)

Uses `clang-format-17` when available (matches CI), otherwise `clang-format`. With **`--dry-run`**, exits non-zero if any file needs formatting (CI-style check).

```bash
python3 scripts/clang_formatter.py all --dry-run
python3 scripts/clang_formatter.py src include tests
python3 scripts/clang_formatter.py --file path/to/file.cpp
```

Omit `--dry-run` to rewrite files in place.
