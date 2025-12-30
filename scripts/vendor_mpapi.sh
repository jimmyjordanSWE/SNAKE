#!/usr/bin/env bash
set -euo pipefail

MPAPI_REPO="https://github.com/robin-onvo/mpapi.git"
VENDOR_DIR="${PWD}/vendor/mpapi"

if [ -d "$VENDOR_DIR" ]; then
    echo "vendor/mpapi already exists at $VENDOR_DIR"
    exit 0
fi

echo "Cloning mpapi into $VENDOR_DIR"
if ! command -v git >/dev/null 2>&1; then
    echo "error: git is required to vendor mpapi" >&2
    exit 1
fi

git clone --depth 1 "$MPAPI_REPO" "$VENDOR_DIR" || { echo "error: git clone failed" >&2; exit 1; }

echo "mpapi vendored into $VENDOR_DIR"
echo "Run './scripts/run_mpapi_server.sh' or 'make mpapi-start' to run the server"
