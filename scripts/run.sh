#!/usr/bin/env bash
# Builds (if needed) and runs PulseTrack, forwarding any CLI flags through,
# e.g.: scripts/run.sh --meter=commercial --duration=15
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"
if [ ! -x ./pulsetrack ]; then
    echo "No build found, building first..."
    make all
fi

./pulsetrack "$@"
