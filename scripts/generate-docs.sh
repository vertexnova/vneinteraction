#!/bin/bash

#==============================================================================
# VneInteraction Documentation Generation Script
# Drives Doxygen API docs via CMake and optionally validates links.
#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#==============================================================================

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DOCS_DIR="$PROJECT_ROOT/docs"
BUILD_DIR="$PROJECT_ROOT/build/shared"
DOXYGEN_HTML="$BUILD_DIR/docs/html"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    if [[ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]]; then
        log_error "Not in VneInteraction project root. Please run from project root."
        exit 1
    fi

    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found. Please install CMake."
        exit 1
    fi

    if ! command -v doxygen &> /dev/null; then
        log_warning "Doxygen not found. API documentation will not be generated."
        DOXYGEN_AVAILABLE=false
    else
        DOXYGEN_AVAILABLE=true
    fi

    if ! command -v markdown-link-check &> /dev/null; then
        log_warning "markdown-link-check not found. Link validation will be skipped."
        LINK_CHECK_AVAILABLE=false
    else
        LINK_CHECK_AVAILABLE=true
    fi

    log_success "Prerequisites check completed"
}

# Generate API documentation via CMake
generate_api_docs() {
    if [[ "$DOXYGEN_AVAILABLE" == "false" ]]; then
        log_warning "Skipping API documentation generation (Doxygen not available)"
        return
    fi

    log_info "Generating API documentation (via CMake)..."

    mkdir -p "$BUILD_DIR"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
        -DENABLE_DOXYGEN=ON -DVNE_INTERACTION_LIB_TYPE=shared
    cmake --build "$BUILD_DIR" --target vneinteraction_doc_doxygen

    # Doxygen output is build/shared/docs/html/index.html (OUTPUT_DIRECTORY=.../docs, HTML_OUTPUT=html)
    if [[ -f "$DOXYGEN_HTML/index.html" ]]; then
        log_success "API documentation generated successfully"
        log_info "API documentation available at: $DOXYGEN_HTML/index.html"
    else
        log_warning "API documentation may have failed (expected at $DOXYGEN_HTML/index.html)"
    fi
}

# Validate documentation links
validate_links() {
    if [[ "$LINK_CHECK_AVAILABLE" == "false" ]]; then
        log_warning "Skipping link validation (markdown-link-check not available)"
        return
    fi

    log_info "Validating documentation links..."

    local failed_links=0

    if [[ -d "$DOCS_DIR" ]]; then
        while IFS= read -r -d '' file; do
            log_info "Checking links in: $file"
            if ! markdown-link-check "$file" 2>/dev/null; then
                ((failed_links++)) || true
            fi
        done < <(find "$DOCS_DIR" -name "*.md" -print0 2>/dev/null || true)
    fi

    if [[ -f "$PROJECT_ROOT/README.md" ]]; then
        log_info "Checking links in: README.md"
        if ! markdown-link-check "$PROJECT_ROOT/README.md" 2>/dev/null; then
            ((failed_links++)) || true
        fi
    fi

    if [[ -f "$PROJECT_ROOT/examples/README.md" ]]; then
        log_info "Checking links in: examples/README.md"
        if ! markdown-link-check "$PROJECT_ROOT/examples/README.md" 2>/dev/null; then
            ((failed_links++)) || true
        fi
    fi

    if [[ $failed_links -eq 0 ]]; then
        log_success "All documentation links are valid"
    else
        log_error "Found $failed_links files with broken links"
        return 1
    fi
}

# Generate documentation coverage report
generate_coverage_report() {
    log_info "Generating documentation coverage report..."

    local total_files=0
    local documented_files=0

    for dir in "$PROJECT_ROOT/include" "$PROJECT_ROOT/src"; do
        if [[ -d "$dir" ]]; then
            while IFS= read -r -d '' file; do
                ((total_files++)) || true
                if grep -q "@brief\|@class\|@function\|@param\|@file" "$file" 2>/dev/null; then
                    ((documented_files++)) || true
                fi
            done < <(find "$dir" \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" \) -print0 2>/dev/null || true)
        fi
    done

    if [[ $total_files -gt 0 ]]; then
        local coverage=$((documented_files * 100 / total_files))
        log_info "Documentation Coverage: $documented_files/$total_files files ($coverage%)"

        if [[ $coverage -lt 80 ]]; then
            log_warning "Documentation coverage is below 80%"
        else
            log_success "Documentation coverage is acceptable"
        fi
    else
        log_warning "No source files found to analyze"
    fi
}

# Main function
main() {
    log_info "Starting VneInteraction documentation generation..."

    check_prerequisites
    generate_api_docs

    if [[ -d "$DOCS_DIR" ]]; then
        validate_links
    fi

    generate_coverage_report

    log_success "Documentation generation completed successfully!"
}

# Help function
show_help() {
    echo "VneInteraction Documentation Generator"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --help, -h     Show this help message"
    echo "  --api-only     Generate only API documentation (via CMake + Doxygen)"
    echo "  --validate     Only validate existing documentation links and coverage"
    echo ""
    echo "Examples:"
    echo "  $0              # Generate API docs, validate links, coverage report"
    echo "  $0 --api-only   # Generate only API docs"
    echo "  $0 --validate   # Only validate links and coverage"
}

# Parse command line arguments
case "${1:-}" in
    --help|-h)
        show_help
        exit 0
        ;;
    --api-only)
        check_prerequisites
        generate_api_docs
        ;;
    --validate)
        check_prerequisites
        validate_links
        generate_coverage_report
        ;;
    "")
        main
        ;;
    *)
        log_error "Unknown option: $1"
        show_help
        exit 1
        ;;
esac
