#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: run_tests.sh [--unit] [--all] [--valgrind] [--help]

Options:
  --unit       Run unit tests only (default if no option given)
  --all        Run full test matrix (equivalent to `make test`)
  --valgrind   Run valgrind after building under `CONFIG=valgrind` with SDL_VIDEODRIVER=dummy
  --help       Show this help
EOF
}

if [ "$#" -eq 0 ]; then
  MODE=unit
else
  MODE="${1:-}"
fi

case "$MODE" in
  --help|-h)
    usage
    exit 0
    ;;
  --unit)
    echo "Running unit tests..."
    make test-unit
    ;;
  --all)
    echo "Running full test suite (unit + integration)..."
    make test-all
    ;;
  --integration)
    echo "Running integration tests..."
    make test-integration
    ;;
  --valgrind)
    echo "Running valgrind (SDL_VIDEODRIVER=dummy)..."
    make test-valgrind
    ;;
  *)
    echo "Unknown option: $MODE" >&2
    usage
    exit 2
    ;;
esac

echo "All requested tests completed."