#!/bin/bash
set -euo pipefail
CORPUS_DIR="build/fuzz/corpus/stb_image"
mkdir -p "$CORPUS_DIR"
# Minimal 1x1 PNG (base64)
cat > "$CORPUS_DIR/1x1.png.b64" <<'B64'
iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR4nGMAAQAABQABDQottwAAAABJRU5ErkJggg==
B64
base64 -d "$CORPUS_DIR/1x1.png.b64" > "$CORPUS_DIR/1x1.png"
rm "$CORPUS_DIR/1x1.png.b64"
echo "Seeded corpus at $CORPUS_DIR/1x1.png"