#!/bin/bash

#==============================================================================
# VneInteraction Linux Build Script
#==============================================================================
# Copyright (c) 2024 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# This script builds VneInteraction for Linux platforms
#==============================================================================

set -e  # Exit on any error

JOBS=10

while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--jobs)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then JOBS="$2"; shift 2; else echo "Invalid number of jobs: $2"; exit 1; fi ;;
        -j*)
            JOBS="${1#-j}"; [[ "$JOBS" =~ ^[0-9]+$ ]] || { echo "Invalid number of jobs: $JOBS"; exit 1; }; shift ;;
        *) ARGS+=("$1"); shift ;;
    esac
done
set -- "${ARGS[@]}"

PLATFORM="Linux"
COMPILER="gcc"

usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-c <compiler>] [-clean] [-interactive] [-j <jobs>]"
  echo "  -t <build_type>  Debug|Release|RelWithDebInfo|MinSizeRel"
  echo "  -a <action>      configure|build|configure_and_build|test"
  echo "  -c <compiler>    gcc|clang"
  echo "  -clean           Clean build directory first"
  echo "  -interactive     Interactive mode"
  echo "  -j <jobs>        Parallel jobs (default: 10)"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
COMPILER="gcc"
CLEAN_BUILD=false
INTERACTIVE_MODE=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -c|--compiler) COMPILER="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    -interactive|--interactive) INTERACTIVE_MODE=true; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

if [ "$COMPILER" = "gcc" ]; then
  COMPILER_VERSION=$(gcc --version | head -n 1 | awk '{print $4}')
elif [ "$COMPILER" = "clang" ]; then
  COMPILER_VERSION=$(clang --version | head -n 1 | awk '{print $3}')
else
  echo "Unsupported compiler: $COMPILER"; exit 1
fi

echo "$PLATFORM :: $COMPILER-${COMPILER_VERSION}"
PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/${BUILD_TYPE}/build-linux-$COMPILER-${COMPILER_VERSION}"

if [ "$COMPILER" = "gcc" ]; then
  CONFIGURE_CMD="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON $PROJECT_ROOT"
elif [ "$COMPILER" = "clang" ]; then
  CONFIGURE_CMD="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_TESTS=ON $PROJECT_ROOT"
fi

BUILD_COMMAND="make -j$JOBS"
TEST_COMMAND="ctest --output-on-failure"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { [ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }

case $ACTION in
  configure) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD ;;
  build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_COMMAND ;;
  configure_and_build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_COMMAND ;;
  test) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_COMMAND; eval $TEST_COMMAND ;;
  *) usage ;;
esac

echo ""
echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
