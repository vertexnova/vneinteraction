#!/usr/bin/env python3
"""
VneTemplate Windows Build Script
Copyright (c) 2024 Ajeet Singh Yadav. All rights reserved.
Licensed under the Apache License, Version 2.0 (the "License")

This script builds VneTemplate for Windows with Visual Studio integration.
"""

import os
import sys
import subprocess
import argparse
import shutil
import re
from pathlib import Path
from typing import List, Optional


class BuildConfig:
    def __init__(self):
        self.jobs = 10
        self.build_type = "Debug"
        self.action = "configure_and_build"
        self.clean_build = False
        self.interactive = False
        self.platform = "Windows"
        self.compiler = "cl"


def find_visual_studio() -> Optional[str]:
    vs_paths = [
        r"C:\Program Files\Microsoft Visual Studio\2022\Community",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\Community",
        r"C:\Program Files\Microsoft Visual Studio\2019\Community",
        r"C:\Program Files\Microsoft Visual Studio\2019\Professional",
    ]
    for vs_path in vs_paths:
        if os.path.exists(vs_path):
            return vs_path
    return None


def get_compiler_version() -> str:
    try:
        result = subprocess.run(["cl"], capture_output=True, text=True, stderr=subprocess.STDOUT)
        match = re.search(r"Version (\d+\.\d+\.\d+)", result.stdout or "")
        if match:
            return match.group(1)
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    vs_path = find_visual_studio()
    if vs_path:
        if "2022" in vs_path:
            return "2022"
        if "2019" in vs_path:
            return "2019"
    return "unknown"


def check_visual_studio_env() -> bool:
    try:
        subprocess.run(["cl"], capture_output=True, check=False)
        return True
    except FileNotFoundError:
        return False


def setup_visual_studio_env():
    vs_path = find_visual_studio()
    if not vs_path:
        print("Error: Visual Studio not found. Please install VS 2019 or 2022.")
        sys.exit(1)
    vsdevcmd = os.path.join(vs_path, "Common7", "Tools", "VsDevCmd.bat")
    if not os.path.exists(vsdevcmd):
        print(f"Error: VsDevCmd.bat not found at {vsdevcmd}")
        sys.exit(1)
    print(f"Found Visual Studio at: {vs_path}")
    print("Please run this script from a Visual Studio Developer Command Prompt")
    print(f"Or run: {vsdevcmd}")
    sys.exit(1)


def check_cmake() -> bool:
    try:
        subprocess.run(["cmake", "--version"], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def interactive_mode(config: BuildConfig):
    print("=== VneTemplate Interactive Build Configuration ===")
    print(f"Detected Platform: {config.platform}\n")
    print("Select Build Type:\n1) Debug (default)\n2) Release\n3) RelWithDebInfo\n4) MinSizeRel")
    build_choice = input("Enter choice (1-4) [1]: ").strip()
    config.build_type = {"2": "Release", "3": "RelWithDebInfo", "4": "MinSizeRel"}.get(build_choice, "Debug")
    print("\nSelect Action:\n1) Configure only\n2) Configure and build (default)\n3) Configure, build, and test")
    action_choice = input("Enter choice (1-3) [2]: ").strip()
    config.action = {"1": "configure", "3": "test"}.get(action_choice, "configure_and_build")
    if input("\nClean build directory before starting? (y/N): ").strip().lower() == "y":
        config.clean_build = True
    jobs_input = input(f"Number of parallel jobs (default: {config.jobs}): ").strip()
    if jobs_input:
        try:
            config.jobs = int(jobs_input)
        except ValueError:
            pass
    print("\n=== Configuration Summary ===")
    print(f"Platform: {config.platform}\nBuild Type: {config.build_type}\nAction: {config.action}\nClean: {config.clean_build}\nJobs: {config.jobs}")
    if input("Proceed? (Y/n): ").strip().lower() == "n":
        print("Build cancelled.")
        sys.exit(0)


def build_cmake_command(project_root: Path, build_type: str) -> List[str]:
    return [
        "cmake", "-G", "Visual Studio 17 2022", "-A", "x64",
        "-DCMAKE_BUILD_TYPE=" + build_type, "-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl",
        "-DVNE_TEMPLATE_TESTS=ON", str(project_root),
    ]


def clean_build_dir(build_dir: Path):
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)


def ensure_build_dir(build_dir: Path):
    build_dir.mkdir(parents=True, exist_ok=True)


def configure_project(build_dir: Path, project_root: Path, build_type: str) -> bool:
    print("Configuring project...")
    try:
        subprocess.run(build_cmake_command(project_root, build_type), cwd=build_dir, check=True)
        return True
    except subprocess.CalledProcessError:
        print("Error: CMake configuration failed")
        return False


def build_project(build_dir: Path, build_type: str, jobs: int) -> bool:
    print(f"Building with {jobs} parallel jobs...")
    try:
        subprocess.run(["cmake", "--build", ".", "--config", build_type, "--parallel", str(jobs)], cwd=build_dir, check=True)
        return True
    except subprocess.CalledProcessError:
        print("Error: Build failed")
        return False


def run_tests(build_dir: Path, build_type: str) -> bool:
    print("Running tests...")
    try:
        subprocess.run(["ctest", "-C", build_type, "--output-on-failure"], cwd=build_dir, check=True)
        return True
    except subprocess.CalledProcessError:
        print("Warning: Some tests failed")
        return False


def main():
    parser = argparse.ArgumentParser(description="Build VneTemplate for Windows")
    parser.add_argument("-t", "--build-type", choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"], default="Debug")
    parser.add_argument("-a", "--action", choices=["configure", "build", "configure_and_build", "test"], default="configure_and_build")
    parser.add_argument("-j", "--jobs", type=int, default=10)
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--interactive", action="store_true")
    args = parser.parse_args()

    config = BuildConfig()
    config.build_type = args.build_type
    config.action = args.action
    config.jobs = args.jobs
    config.clean_build = args.clean
    config.interactive = args.interactive

    if config.interactive:
        interactive_mode(config)

    if not check_visual_studio_env():
        setup_visual_studio_env()
    if not check_cmake():
        print("Error: CMake not found in PATH")
        sys.exit(1)

    compiler_version = get_compiler_version()
    print(f"{config.platform} :: {config.compiler}-{compiler_version}")

    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    build_dir = project_root / "build" / config.build_type / f"build-windows-{config.compiler}-{compiler_version}"

    if config.clean_build:
        clean_build_dir(build_dir)
    else:
        ensure_build_dir(build_dir)

    if config.action in ["configure", "build", "configure_and_build", "test"]:
        if not configure_project(build_dir, project_root, config.build_type):
            sys.exit(1)
    if config.action in ["build", "configure_and_build", "test"]:
        if not build_project(build_dir, config.build_type, config.jobs):
            sys.exit(1)
    if config.action == "test":
        run_tests(build_dir, config.build_type)

    print("\n=== Build completed successfully ===")
    print(f"Build directory: {build_dir}")


if __name__ == "__main__":
    main()
