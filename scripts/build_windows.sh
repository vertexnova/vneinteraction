#!/bin/bash
# VneInteraction Windows Build Script
# Copyright (c) 2024 Ajeet Singh Yadav. Licensed under the Apache License, Version 2.0

set -e
JOBS=10
while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--jobs) [[ -n "$2" && "$2" =~ ^[0-9]+$ ]] && { JOBS="$2"; shift 2; } || { echo "Invalid jobs: $2"; exit 1; } ;;
        -j*) JOBS="${1#-j}"; [[ "$JOBS" =~ ^[0-9]+$ ]] || { echo "Invalid jobs: $JOBS"; exit 1; }; shift ;;
        *) ARGS+=("$1"); shift ;;
    esac
done
set -- "${ARGS[@]}"

usage() { echo "Usage: $0 [-t <build_type>] [-a <action>] [-clean] [-j <jobs>]"; exit 1; }
BUILD_TYPE="Debug"
ACTION="configure_and_build"
CLEAN_BUILD=false
while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

if ! command -v cl &> /dev/null; then
  echo "Error: Visual Studio compiler 'cl' not found. Run from a VS Developer Command Prompt."; exit 1
fi
if ! command -v cmake &> /dev/null; then echo "Error: CMake not found in PATH"; exit 1; fi

COMPILER_VERSION="unknown"
[ -d "/c/Program Files/Microsoft Visual Studio/2022" ] && COMPILER_VERSION="2022"
[ -d "/c/Program Files/Microsoft Visual Studio/2019" ] && COMPILER_VERSION="2019"
echo "Windows :: cl-${COMPILER_VERSION}"
PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/${BUILD_TYPE}/build-windows-cl-${COMPILER_VERSION}"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { [ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }

configure_project() { cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTS=ON "$PROJECT_ROOT" || exit 1; }
build_project() { cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS" || exit 1; }
run_tests() { ctest -C "$BUILD_TYPE" --output-on-failure; }

case $ACTION in
  configure) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project ;;
  build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project; build_project ;;
  configure_and_build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project; build_project ;;
  test) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project; build_project; run_tests ;;
  *) usage ;;
esac
echo ""; echo "=== Build completed successfully ==="; echo "Build directory: $BUILD_DIR"
