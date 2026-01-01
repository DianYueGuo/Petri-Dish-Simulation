#!/usr/bin/env bash
set -euo pipefail

CONFIG="clang-uml.yml"
OUT_DIR="uml"

# Relationship-only overview (names/edges)
OVERVIEW_DIAGRAMS=(
  petri_component_overview
)

# Simplified flow (sequence) diagrams
FLOW_DIAGRAMS=(
  petri_flow_simple
)

# Detailed class diagrams
DETAIL_DIAGRAMS=(
  petri_class_diagram
  petri_game_details
  petri_circles_details
  class_CirclePhysics
  class_DrawableCircle
  class_EatableCircle
  class_CircleRegistry
  class_CreatureCircle
)

run_diagrams() {
  local label="$1"; shift
  local diagrams=("$@")
  for d in "${diagrams[@]}"; do
    echo "Generating ${label}: ${d}"
    clang-uml -c "${CONFIG}" -n "${d}" -q
  done
}

render_plantuml() {
  if command -v plantuml >/dev/null 2>&1; then
    echo "Rendering PlantUML diagrams to SVG..."
    plantuml -tsvg "${OUT_DIR}"/*.puml
  else
    echo "PlantUML not found on PATH; skipping SVG rendering."
  fi
}

mkdir -p "${OUT_DIR}"

run_diagrams "overview" "${OVERVIEW_DIAGRAMS[@]}"
run_diagrams "flow" "${FLOW_DIAGRAMS[@]}"
run_diagrams "details" "${DETAIL_DIAGRAMS[@]}"

render_plantuml
