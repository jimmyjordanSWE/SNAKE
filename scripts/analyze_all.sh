#!/bin/bash
# analyze_all.sh - Run all static analysis scripts
set -e

VENV_PYTHON="./.venv/bin/python3"
SCRIPTS_DIR="scripts"
OUT_DIR="scripts/out"

# Ensure venv exists
if [ ! -f "$VENV_PYTHON" ]; then
    echo "Error: Virtual environment not found at .venv"
    exit 1
fi

mkdir -p "$OUT_DIR"

echo "Refreshing LLM Context (Static Analysis)..."

echo "=== Structure ==="
$VENV_PYTHON "$SCRIPTS_DIR/structure.py" > "$OUT_DIR/structure_out.txt"

scripts=(
    "memory_map.py"
    "call_chains.py"
    "errors.py"
    "data_flow.py"
    "dependencies.py"
    "macros.py"
    "hotspots.py"
    "invariants.py"
)

for script in "${scripts[@]}"; do
    if [ -f "$SCRIPTS_DIR/$script" ]; then
        name="${script%.py}"
        echo "- refreshing $name context..."
        $VENV_PYTHON "$SCRIPTS_DIR/$script" > "$OUT_DIR/${name}_out.txt"
    fi
done


echo "=== AST Context ==="
$VENV_PYTHON "$SCRIPTS_DIR/structure.py" > "PROJECT_CONTEXT.txt"

echo -e "\nLLM Context Updated. Results in $OUT_DIR/ and PROJECT_CONTEXT.txt"
