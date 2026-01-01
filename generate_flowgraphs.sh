#!/usr/bin/env bash
set -euo pipefail

CONFIG="Doxyfile.flow"
OUT_DIR="uml-flow"

if ! command -v doxygen >/dev/null 2>&1; then
  echo "doxygen not found on PATH. Install it to generate flow graphs." >&2
  exit 1
fi

if ! command -v dot >/dev/null 2>&1; then
  echo "graphviz 'dot' not found on PATH. Install it for SVG call graphs." >&2
  exit 1
fi

echo "Generating call/flow graphs via Doxygen (${CONFIG})..."
doxygen "${CONFIG}"

echo "Done. Open ${OUT_DIR}/html/index.html and follow the call graphs."
