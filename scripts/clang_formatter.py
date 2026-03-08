#!/usr/bin/env python3
"""
VneLogging Clang Formatter

A generic script to format C/C++ source files using clang-format with
VneLogging-specific formatting rules.

Usage:
    python scripts/clang_formatter.py <folder_path> [options]
    python scripts/clang_formatter.py --file <file_path> [options]

Examples:
    python scripts/clang_formatter.py src/vnelogging
    python scripts/clang_formatter.py src/vnelogging --dry-run
    python scripts/clang_formatter.py src/vnelogging --verbose
    python scripts/clang_formatter.py --file src/vertexnova/logging/logging.cpp
    python scripts/clang_formatter.py --file include/vertexnova/logging/logging.h
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


def find_source_files(folder_path: Path) -> list:
    """Find all C/C++ source files in the specified folder."""
    source_files = []

    # Supported file extensions
    extensions = ('.h', '.cpp', '.mm', '.m', '.hpp', '.c')

    # Directories to exclude
    exclude_dirs = {'build', '.git', 'node_modules', 'CMakeFiles', '__pycache__'}

    for root, dirs, files in os.walk(folder_path):
        # Remove excluded directories from dirs list to prevent walking into them
        dirs[:] = [d for d in dirs if d not in exclude_dirs]

        for file in files:
            if file.endswith(extensions):
                source_files.append(os.path.join(root, file))

    return source_files


def is_source_file(file_path: Path) -> bool:
    """Check if a file is a supported source file."""
    extensions = ('.h', '.cpp', '.mm', '.m', '.hpp', '.c')
    return file_path.suffix.lower() in extensions


def check_clang_format() -> bool:
    """Check if clang-format is available."""
    try:
        subprocess.run(['clang-format', '--version'],
                      capture_output=True,
                      check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def run_clang_format(files: list, style_file: Path, dry_run: bool = False) -> bool:
    """Run clang-format on the specified files."""
    if not files:
        print("No source files found to format.")
        return True

    print(f"Found {len(files)} source files to format.")

    if dry_run:
        print("DRY RUN - No files will be modified.")
        return True

    # Run clang-format on all files
    cmd = [
        'clang-format',
        '-i',  # In-place formatting
        '--style=file',  # Use .clang-format file
        '--fallback-style=Google'  # Fallback style
    ]

    for file_path in files:
        print(f"Formatting: {file_path}")
        try:
            result = subprocess.run(cmd + [file_path],
                                  capture_output=True,
                                  text=True,
                                  check=True)
            print(f"  ✓ Formatted successfully")
        except subprocess.CalledProcessError as e:
            print(f"  ✗ Error formatting {file_path}: {e}")
            if e.stderr:
                print(f"    stderr: {e.stderr}")
            return False
        except FileNotFoundError:
            print(f"  ✗ clang-format not found. Please install clang-format.")
            return False

    return True


def main():
    """Main function."""
    parser = argparse.ArgumentParser(
        description="Format C/C++ source files using clang-format with VneLogging rules",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python scripts/clang_formatter.py src/vnelogging
  python scripts/clang_formatter.py src/vnelogging --dry-run
  python scripts/clang_formatter.py src/vnelogging --verbose
  python scripts/clang_formatter.py tests
  python scripts/clang_formatter.py --file src/vertexnova/logging/logging.cpp
  python scripts/clang_formatter.py --file include/vertexnova/logging/logging.h
        """
    )

    # Create mutually exclusive group for folder vs file
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        'folder',
        nargs='?',
        help='Folder path to format (e.g., src/vnelogging)'
    )
    group.add_argument(
        '--file',
        help='Specific file to format (e.g., src/vertexnova/logging/logging.cpp)'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be formatted without making changes'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Show verbose output'
    )

    args = parser.parse_args()

    # Get the project root directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    style_file = project_root / ".clang-format"

    # Check if .clang-format exists
    if not style_file.exists():
        print(f"Warning: .clang-format not found at {style_file}")
        print("Using fallback style: Google")

    print("VneLogging Clang Formatter")
    print("=" * 40)
    if style_file.exists():
        print(f"Style file: {style_file}")
    print()

    if args.file:
        # Format specific file
        target_file = project_root / args.file

        if not target_file.exists():
            print(f"Error: File not found at {target_file}")
            sys.exit(1)

        if not is_source_file(target_file):
            print(f"Error: {target_file} is not a supported source file type.")
            print("Supported extensions: .h, .cpp, .mm, .m, .hpp, .c")
            sys.exit(1)

        print(f"Target file: {target_file}")
        print()

        source_files = [str(target_file)]

    else:
        # Format folder
        target_folder = project_root / args.folder

        if not target_folder.exists():
            print(f"Error: Target folder not found at {target_folder}")
            sys.exit(1)

        print(f"Target folder: {target_folder}")
        print()

        # Find source files
        source_files = find_source_files(target_folder)

    if args.verbose:
        print("Source files found:")
        for file in source_files:
            print(f"  {file}")
        print()

    # Check clang-format availability
    if not check_clang_format():
        print("Error: clang-format not found. Please install clang-format.")
        print("  Ubuntu/Debian: sudo apt install clang-format")
        print("  macOS: brew install clang-format")
        sys.exit(1)

    # Run clang-format
    success = run_clang_format(source_files, style_file, args.dry_run)

    if success:
        print("\n✓ Formatting completed successfully!")
        if not args.dry_run:
            if args.file:
                print(f"Formatted 1 file: {args.file}")
            else:
                print(f"Formatted {len(source_files)} files in {args.folder}")
    else:
        print("\n✗ Formatting failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
