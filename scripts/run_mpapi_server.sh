#!/usr/bin/env bash
set -euo pipefail

MPAPI_REPO="https://github.com/robin-onvo/mpapi.git"
VENDOR_DIR="${PWD}/vendor/mpapi"
BUILD_DIR="${PWD}/build/mpapi-server"

# Prefer vendored copy if present
if [ -d "$VENDOR_DIR" ]; then
    REPO_DIR="$VENDOR_DIR"
else
    REPO_DIR="$BUILD_DIR"
fi

if ! command -v node >/dev/null 2>&1; then
    echo "error: node is required to run local mpapi server" >&2
    exit 1
fi

if [ -d "$REPO_DIR" ]; then
    echo "Using mpapi from $REPO_DIR"
else
    echo "Cloning mpapi into $REPO_DIR"
    git clone "$MPAPI_REPO" "$REPO_DIR" || { echo "error: git clone failed" >&2; exit 1; }
    (cd "$REPO_DIR" && npm install --no-audit --no-fund) || { echo "error: npm install failed" >&2; exit 1; }
fi

cd "$REPO_DIR"
PORT=${1:-8080}
TCP_PORT=${2:-9001}

echo "Ensuring ports $PORT and $TCP_PORT are free..."
fuser -k "$PORT/tcp" >/dev/null 2>&1 || true
fuser -k "$TCP_PORT/tcp" >/dev/null 2>&1 || true

export HTTP_PORT=$PORT
export HTTPS_PORT=0
export FORCE_HTTPS=false

echo "Starting mpapi server (HTTP port=$HTTP_PORT, TCP port=$TCP_PORT)"
node backend/index.js &
PID=$!
echo "mpapi server started with PID $PID"

echo "To stop: kill $PID"