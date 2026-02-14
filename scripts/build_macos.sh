#!/bin/bash
# VneInteraction macOS Build Script
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

usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-clean] [-interactive] [-j <jobs>] [-xcode] [-xcode-only]"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
CLEAN_BUILD=false
GENERATE_XCODE=false
XCODE_ONLY=false
while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    -interactive|--interactive) shift ;;  # optional
    -xcode|--xcode) GENERATE_XCODE=true; shift ;;
    -xcode-only|--xcode-only) XCODE_ONLY=true; GENERATE_XCODE=true; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

[ "$XCODE_ONLY" = true ] && ACTION="xcode"
[ "$GENERATE_XCODE" = true ] && [ "$ACTION" = "configure_and_build" ] && ACTION="xcode_build"
[ "$ACTION" = "xcode" ] || [ "$ACTION" = "xcode_build" ] && GENERATE_XCODE=true

COMPILER_VERSION=$(clang --version | head -n 1 | awk '{print $4}' | sed 's/(.*)//')
[ "$COMPILER_VERSION" = "version" ] && COMPILER_VERSION=$(clang --version | head -n 1 | awk '{print $3}')
echo "Darwin :: clang-${COMPILER_VERSION}"
PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)

if [ "$GENERATE_XCODE" = true ]; then
  BUILD_DIR="$PROJECT_ROOT/build/${BUILD_TYPE}/xcode-macos-clang-${COMPILER_VERSION}"
else
  BUILD_DIR="$PROJECT_ROOT/build/${BUILD_TYPE}/build-macos-clang-${COMPILER_VERSION}"
fi

[ "$GENERATE_XCODE" = true ] && CONFIGURE_CMD="cmake -G Xcode -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DBUILD_TESTS=ON $PROJECT_ROOT" \
  || CONFIGURE_CMD="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DBUILD_TESTS=ON $PROJECT_ROOT"

[ "$GENERATE_XCODE" = true ] && BUILD_CMD="xcodebuild -project vneinteraction.xcodeproj -configuration $BUILD_TYPE -parallelizeTargets -jobs $JOBS" && TEST_CMD="xcodebuild -project vneinteraction.xcodeproj -configuration $BUILD_TYPE -target RUN_TESTS" \
  || BUILD_CMD="make -j$JOBS" && TEST_CMD="ctest --output-on-failure"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { [ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }

case $ACTION in
  configure) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD ;;
  build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_CMD ;;
  configure_and_build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_CMD ;;
  test) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_CMD; eval $TEST_CMD ;;
  xcode) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; echo "Xcode project: $BUILD_DIR (vneinteraction.xcodeproj)" ;;
  xcode_build) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; eval $CONFIGURE_CMD; eval $BUILD_CMD; echo "Xcode build done: $BUILD_DIR" ;;
  *) usage ;;
esac
echo ""; echo "=== Build completed successfully ==="; echo "Build directory: $BUILD_DIR"
