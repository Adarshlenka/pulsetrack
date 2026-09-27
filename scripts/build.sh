#!/usr/bin/env bash
# Builds PulseTrack. Run from anywhere; this script locates the project
# root relative to itself so it doesn't depend on the caller's cwd.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"
echo "Building PulseTrack in $PROJECT_ROOT ..."
make all
echo "Build complete: $PROJECT_ROOT/pulsetrack"
