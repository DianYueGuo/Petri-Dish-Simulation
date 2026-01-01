#!/usr/bin/env bash
set -euo pipefail

CONFIG="clang-uml.yml"
OUT_DIR="uml"

# Relationship-only overview (class names only, no members)
RELATIONSHIP_DIAGRAMS=(
  petri_component_overview
)

# High-level simplified flow diagrams
FLOW_DIAGRAMS=(
  petri_flow_simple
)

# System-level detailed class views
SYSTEM_DETAIL_DIAGRAMS=(
  petri_class_diagram
  petri_game_details
  petri_circles_details
)

# Detailed diagrams for each individual class (members + functions)
INDIVIDUAL_CLASS_DIAGRAMS=(
  class_CirclePhysics
  class_DrawableCircle
  class_EatableCircle
  class_CircleRegistry
  class_CreatureCircle
  class_ContactGraph
  class_SelectionManager
  class_Spawner
  class_Game
  class_GameInputHandler
  class_GameSelectionController
  class_GamePopulationManager
  class_GameSimulationController
  class_UiFacade
  class_ISenseable
  class_IEdible
  class_neat_Genome
  class_neat_Node
  class_neat_Connection
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

run_diagrams "relationship" "${RELATIONSHIP_DIAGRAMS[@]}"
run_diagrams "flow" "${FLOW_DIAGRAMS[@]}"
run_diagrams "system detail" "${SYSTEM_DETAIL_DIAGRAMS[@]}"
run_diagrams "individual class detail" "${INDIVIDUAL_CLASS_DIAGRAMS[@]}"

render_plantuml
