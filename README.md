# Evolutionary Agar.io-like 2D Game  
**Technologies:** SFML • ImGui • Box2D • C++  
**Concept:** Evolving cells controlled by neural node-graphs, reproducing, mutating, emitting RGB signals, sensing 16 directions, consuming mass to survive.

---

# Project Roadmap

This roadmap defines the incremental path to complete the game.  
Each phase should compile, run, and be stable before continuing.

---

## Phase 0 — Project Skeleton

**Goal:** Integrate SFML + ImGui + Box2D cleanly.

- Set up build system (CMake recommended).
- Create SFML window.
- Add ImGui overlay window.
- Initialize Box2D world.
- Spawn a single circle body and step physics.
- Render via SFML and show an ImGui window.

**Completion Check:**  
A window appears with a Box2D body being simulated and an ImGui panel visible.

---

## Phase 1 — Core World: Cells, Physics, Eating, Camera

**Goal:** Basic world simulation with player-controlled cells.

- Add `Cell` and `Food` structs.
- Create one Box2D body per cell (circular fixture, usually sensor-only).
- Allow manual spawning of cells at cursor.
- Implement keyboard control: thrust + torque.
- Implement simple eating logic:
  - If cell radius > distance to food → consume it.
  - Increase mass → recalc radius.
- Add camera controls (pan, zoom, follow selected cell).

**Completion Check:**  
You can spawn a cell, move it, eat food, and watch it grow.

---

## Phase 2 — 16-Direction Sensing (No Neural Network Yet)

**Goal:** Create the sensing system used by the neural network.

- Implement 16 directional RGB sensor bins for each cell.
- For each detected neighbor, compute angle and fill the appropriate bin.
- Draw 16 sensor points around each cell as visualization.
- Implement hard-coded behavior (e.g., move toward green food).

**Completion Check:**  
Visual 16-direction sensors work, and a cell can follow food using them.

---

## Phase 3 — Neural Graph System (Node-Based Brain)

**Goal:** Replace hard-coded logic with neural node graph.

- Define node types (Input, Output, Sum, Multiply, Ramp, Sin, Timer, Random, Constant).
- Implement `NeuralGraph` with:
  - Nodes
  - Input index mapping
  - Output index mapping
- Implement graph evaluation (topological order).
- Create small hand-crafted brains for testing.

**Completion Check:**  
A cell’s movement is driven entirely by its neural graph.

---

## Phase 4 — Energy, Mass, Reproduction, Mutation

**Goal:** Introduce evolution and self-replication.

- Add energy/mass metabolism:
  - Costs for thrust, torque, signal emission, and living.
- Eating restores mass.
- Reproduction logic:
  - If reproduce output > threshold and mass > minimum:
    - Split mass, create child.
    - Copy neural graph → mutate.
- Mutation operators:
  - Weight perturbation  
  - Add/remove connection  
  - Add/remove node  
  - Node type mutation  
- Add generation and parent tracking.

**Completion Check:**  
Cells live, move, eat, shrink, reproduce, and mutate over generations.

---

## Phase 5 — ImGui Control Panels and Debug Tools

**Goal:** Enable simulation control, inspection, and debugging.

- Global controls:
  - Play / Pause / Step
  - Time acceleration
  - Mutation rates
- Cell inspector:
  - ID, parent ID, generation, mass, energy
  - Current outputs
  - Current sensor values
  - Buttons: Kill, Clone, Follow
- Brain viewer:
  - Table listing nodes, input indices, weights, value
- Lineage viewer:
  - Parent chain and children list

**Completion Check:**  
You can inspect any cell in detail and adjust simulation parameters at runtime.

---

## Phase 6 — RGB Signal Particles

**Goal:** Cells can communicate via colored particles.

- Add `Signal` entity (position, velocity, RGB, lifetime).
- Add neural output for signal emission.
- Deduct energy/mass when emitting.
- Sensing treats signals like local colored objects.
- Add decay and removal of old particles.

**Completion Check:**  
Cells emit colored particles that influence others' sensors.

---

## Phase 7 — Save/Load & Replay System

**Goal:** Persist entire simulation states.

- Choose save format (JSON recommended).
- Serialize:
  - Global params
  - All cells (mass, energy, brain)
  - Food particles
  - Signal particles
  - Random seed
- Rebuild Box2D world on load.
- Add ImGui buttons for save/load.
- Optional: autosaves or time-lapse replay.

**Completion Check:**  
You can save the world, reload it later, and continue the simulation.

---

## Phase 8 — Player Control, First-Person Camera, Polish

**Goal:** Make the simulation engaging and user-friendly.

- Allow user to “possess” a cell; override neural outputs with player input.
- Implement follow camera and optional first-person view.
- Improve visuals (better outlines, fading, indicators).
- Add scenario presets and configurable world settings.
- Optimization passes (spatial partitioning, entity limits).

**Completion Check:**  
The project is interactive, visually readable, and enjoyable to explore.

---

# Summary

This roadmap ensures:

1. A stable codebase from the beginning.  
2. Features added in clean, manageable layers.  
3. A working evolutionary ecosystem with clarity and tooling.  

Follow the phases strictly and the project will remain maintainable and scalable.
