#!/bin/bash

#==============================================================================
# VneTemplate Linux Build Script
#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#
# This script builds VneTemplate for Linux platforms
#==============================================================================

set -e  # Exit on any error

# Default parallel jobs
JOBS=10

# Parse command line arguments for jobs
while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--jobs)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then
                JOBS="$2"
                shift 2
            else
                echo "Invalid number of jobs: $2"
                exit 1
            fi
            ;;
        -j*)
            JOBS="${1#-j}"
            if [[ ! "$JOBS" =~ ^[0-9]+$ ]]; then
                echo "Invalid number of jobs: $JOBS"
                exit 1
            fi
            shift
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done

# Reconstruct ARGS for the rest of the script
set -- "${ARGS[@]}"

# Platform detection (force Linux)
PLATFORM="Linux"
COMPILER="gcc"

# Function to display usage information
usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-c <compiler>] [-l <lib_type>] [-clean] [-interactive] [-j <jobs>]"
  echo "Options:"
  echo "  -t <build_type>    Specify the build type (Debug, Release, RelWithDebInfo, MinSizeRel)"
  echo "  -a <action>        Specify the action to perform:"
  echo "                     configure: Configure the project"
  echo "                     build: Build the project"
  echo "                     configure_and_build: Configure and build the project"
  echo "                     test: Run tests"
  echo "  -c <compiler>      Specify the compiler (gcc, clang)"
  echo "  -l <lib_type>      Library type: static or shared (default: shared). Build dir: build/<lib_type>/..."
  echo "  -clean             Clean the build directory before performing the action"
  echo "  -interactive       Run in interactive mode to choose options"
  echo "  -j <jobs>          Number of parallel jobs (default: 10)"
  echo ""
  echo "Examples:"
  echo "  $0 -t Debug -a configure_and_build"
  echo "  $0 -c clang -l static -j 32"
  echo "  $0 -interactive"
  echo "  $0 -t Release -l shared -clean"
  exit 1
}

# Function to run interactive mode
interactive_mode() {
  echo "=== VneTemplate Interactive Build Configuration ==="
  echo ""
  echo "Detected Platform: $PLATFORM"
  echo ""

  # Build type selection
  echo "Select Build Type:"
  echo "1) Debug (default) - Development with debug symbols"
  echo "2) Release - Optimized for production"
  echo "3) RelWithDebInfo - Release with debug info"
  echo "4) MinSizeRel - Minimum size release"
  read -p "Enter choice (1-4) [1]: " build_choice
  case $build_choice in
    2) BUILD_TYPE="Release" ;;
    3) BUILD_TYPE="RelWithDebInfo" ;;
    4) BUILD_TYPE="MinSizeRel" ;;
    *) BUILD_TYPE="Debug" ;;
  esac

  # Compiler selection
  echo ""
  echo "Select Compiler:"
  echo "1) GCC (default) - GNU Compiler Collection"
  echo "2) Clang - LLVM Compiler"
  read -p "Enter choice (1-2) [1]: " compiler_choice
  case $compiler_choice in
    2) COMPILER="clang" ;;
    *) COMPILER="gcc" ;;
  esac

  # Library type (static or shared; build dir will be build/<lib_type>/...)
  echo ""
  echo "Select library type:"
  echo "1) shared (default) - shared library"
  echo "2) static - static library"
  read -p "Enter choice (1-2) [1]: " lib_choice
  case $lib_choice in
    2) LIB_TYPE="static" ;;
    *) LIB_TYPE="shared" ;;
  esac

  # Action selection
  echo ""
  echo "Select Action:"
  echo "1) Configure only"
  echo "2) Configure and build (default)"
  echo "3) Configure, build, and test"
  read -p "Enter choice (1-3) [2]: " action_choice
  case $action_choice in
    1) ACTION="configure" ;;
    3) ACTION="test" ;;
    *) ACTION="configure_and_build" ;;
  esac

  # Clean build option
  echo ""
  read -p "Clean build directory before starting? (y/N): " clean_choice
  if [[ $clean_choice =~ ^[Yy]$ ]]; then
    CLEAN_BUILD=true
  fi

  echo ""
  echo "=== Configuration Summary ==="
  echo "Platform: $PLATFORM"
  echo "Build Type: $BUILD_TYPE"
  echo "Library Type: $LIB_TYPE (build dir: build/$LIB_TYPE/...)"
  echo "Compiler: $COMPILER"
  echo "Action: $ACTION"
  echo "Clean Build: $CLEAN_BUILD"
  echo "Parallel Jobs: $JOBS"
  echo ""
  read -p "Proceed with this configuration? (Y/n): " proceed
  if [[ $proceed =~ ^[Nn]$ ]]; then
    echo "Build cancelled."
    exit 0
  fi
}

# Default values
BUILD_TYPE="Debug"
ACTION="configure_and_build"
COMPILER="gcc"
LIB_TYPE="shared"
CLEAN_BUILD=false
INTERACTIVE_MODE=false

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type)
      BUILD_TYPE="$2"
      shift 2
      ;;
    -a|--action)
      ACTION="$2"
      shift 2
      ;;
    -c|--compiler)
      COMPILER="$2"
      shift 2
      ;;
    -l|--lib-type)
      LIB_TYPE="$2"
      shift 2
      ;;
    -clean|--clean)
      CLEAN_BUILD=true
      shift
      ;;
    -interactive|--interactive)
      INTERACTIVE_MODE=true
      shift
      ;;
    -h|--help)
      usage
      ;;
    *)
      echo "Unknown option: $1"
      usage
      ;;
  esac
done

# Check for interactive mode
if [ "$INTERACTIVE_MODE" = true ]; then
  interactive_mode
fi

# Determine compiler version
if [ "$COMPILER" = "gcc" ]; then
  COMPILER_VERSION=$(gcc --version | head -n 1 | awk '{print $4}')
elif [ "$COMPILER" = "clang" ]; then
  COMPILER_VERSION=$(clang --version | head -n 1 | awk '{print $3}')
else
  echo "Unsupported compiler: $COMPILER"
  exit 1
fi

echo "$PLATFORM :: $COMPILER-${COMPILER_VERSION}"

# Store project root directory
PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)

# Set the build directory: build/<lib_type>/<build_type>/build-linux-<compiler>-<version>
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-linux-$COMPILER-${COMPILER_VERSION}"

# Build CMake command
build_cmake_command() {
  local base_cmd=""

  if [ "$COMPILER" = "gcc" ]; then
    base_cmd="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DVNE_TEMPLATE_LIB_TYPE=$LIB_TYPE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DVNE_TEMPLATE_TESTS=ON"
  elif [ "$COMPILER" = "clang" ]; then
    base_cmd="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DVNE_TEMPLATE_LIB_TYPE=$LIB_TYPE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DVNE_TEMPLATE_TESTS=ON"
  fi

  echo "$base_cmd $PROJECT_ROOT"
}

CONFIGURE_COMMAND=$(build_cmake_command)
BUILD_COMMAND="make -j$JOBS"
TEST_COMMAND="ctest --output-on-failure"

# Clean previous build
clean_build() {
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR" || exit
}

# Ensure build directory exists (if not cleaning)
ensure_build_dir() {
  if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
  fi
  cd "$BUILD_DIR" || exit
}

# Configure the project
configure_project() {
  echo "Configuring with command: $CONFIGURE_COMMAND"
  eval $CONFIGURE_COMMAND
}

# Build the project
build_project() {
  echo "Building with $JOBS parallel jobs..."
  eval $BUILD_COMMAND
}

# Run tests
run_tests() {
  eval $TEST_COMMAND
}

# Perform actions based on the specified option
case $ACTION in
  configure)
    if [ "$CLEAN_BUILD" = true ]; then
      clean_build
    else
      ensure_build_dir
    fi
    configure_project
    ;;
  build)
    if [ "$CLEAN_BUILD" = true ]; then
      clean_build
    else
      ensure_build_dir
    fi
    configure_project
    build_project
    ;;
  configure_and_build)
    if [ "$CLEAN_BUILD" = true ]; then
      clean_build
    else
      ensure_build_dir
    fi
    configure_project
    build_project
    ;;
  test)
    if [ "$CLEAN_BUILD" = true ]; then
      clean_build
    else
      ensure_build_dir
    fi
    configure_project
    build_project
    run_tests
    ;;
  *)
    usage
    ;;
esac

echo ""
echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
