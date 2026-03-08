#!/bin/bash

#==============================================================================
# VneInteraction Windows Build Script
#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#
# This script builds VneInteraction for Windows with Visual Studio integration
#==============================================================================

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

PLATFORM="Windows"
COMPILER="cl"

usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-l <lib_type>] [-clean] [-interactive] [-j <jobs>]"
  echo "  -t <build_type>  Debug|Release|RelWithDebInfo|MinSizeRel"
  echo "  -a <action>      configure|build|configure_and_build|test"
  echo "  -l <lib_type>    static|shared (default: shared). Build dir: build/<lib_type>/..."
  echo "  -clean           Clean build directory first"
  echo "  -interactive     Interactive mode"
  echo "  -j <jobs>        Parallel jobs (default: 10)"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="shared"
CLEAN_BUILD=false
INTERACTIVE_MODE=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    -interactive|--interactive) INTERACTIVE_MODE=true; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

if ! command -v cl &> /dev/null; then
  echo "Error: Visual Studio compiler 'cl' not found in PATH"
  echo "Please run this script from a Visual Studio Developer Command Prompt"
  echo "Or run: 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\VsDevCmd.bat'"
  exit 1
fi

if ! command -v cmake &> /dev/null; then
  echo "Error: CMake not found in PATH"
  exit 1
fi

COMPILER_VERSION="unknown"
if COMPILER_OUTPUT=$(cl 2>&1 | grep -i "version" | head -1); then
  COMPILER_VERSION=$(echo "$COMPILER_OUTPUT" | awk '{print $3}' | sed 's/[^0-9.]//g')
fi
[ -z "$COMPILER_VERSION" ] && [ -d "/c/Program Files/Microsoft Visual Studio/2022" ] && COMPILER_VERSION="2022"
[ -z "$COMPILER_VERSION" ] && [ -d "/c/Program Files/Microsoft Visual Studio/2019" ] && COMPILER_VERSION="2019"

echo "$PLATFORM :: $COMPILER-${COMPILER_VERSION}"
PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
# Build dir: build/<lib_type>/<build_type>/build-windows-...
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-windows-$COMPILER-${COMPILER_VERSION}"

build_cmake_command() {
  cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DVNE_TEMPLATE_LIB_TYPE="$LIB_TYPE" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DVNE_TEMPLATE_TESTS=ON "$PROJECT_ROOT"
}

BUILD_COMMAND="cmake --build . --config $BUILD_TYPE --parallel $JOBS"
TEST_COMMAND="ctest -C $BUILD_TYPE --output-on-failure"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { [ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }

configure_project() { echo "Configuring..."; build_cmake_command || { echo "CMake configuration failed"; exit 1; }; }
build_project() { echo "Building with $JOBS jobs..."; eval $BUILD_COMMAND || { echo "Build failed"; exit 1; }; }
run_tests() { eval $TEST_COMMAND; }

case $ACTION in
  configure) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project ;;
  build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project; build_project ;;
  configure_and_build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project; build_project ;;
  test) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; configure_project; build_project; run_tests ;;
  *) usage ;;
esac

echo ""
echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
