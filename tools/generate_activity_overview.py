#!/usr/bin/env python3
"""
Generate a high-level activity overview diagram from the codebase using
clang-uml output. The overview is derived from the `petri_flow_simple`
sequence diagram, keeping only non-trivial actions and collapsing repeated
getters into a single step per action label.

Outputs:
  - uml/petri_flow_activity_overview.puml
  - uml/petri_flow_activity_overview.svg (if plantuml available)
  - uml/petri_flow_activity_overview.png (if plantuml available)
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UML_DIR = ROOT / "uml"
SEQ_NAME = "petri_flow_simple"
SEQ_PUML = UML_DIR / f"{SEQ_NAME}.puml"
ACT_PUML = UML_DIR / "petri_flow_activity_overview.puml"
ACT_SVG = ACT_PUML.with_suffix(".svg")
ACT_PNG = ACT_PUML.with_suffix(".png")


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True)


def clean_message(text: str) -> str:
    # Strip bold markers and bracket noise clang-uml adds around conditions.
    text = re.sub(r"[*\\[\\]]", "", text)
    text = text.strip()
    return text


def is_noise_action(msg: str) -> bool:
    """Heuristic filter to keep the overview compact (pre-mapping)."""
    if not msg:
        return True
    lower = msg.lower()
    if lower.startswith(("get_", "is_", "world_id", "operator", "activate", "deactivate")):
        return True
    if " const" in lower:
        return True
    if msg in {"", " ", ":"}:
        return True
    return False


def map_action(msg: str) -> str | None:
    """
    Map detailed messages to higher-level buckets.
    Only a handful of coarse actions should survive.
    """
    msg_lower = msg.lower()
    groups: list[tuple[list[str], str]] = [
        (["game(", "selectionmanager", "spawner("], "Initialize core systems"),
        (["spawn", "pellet", "create_creature", "random_point", "spawn_once"], "Prepare initial spawns"),
        (["imgui::sfml::init"], "Initialize UI"),
        (["accumulate", "real time", "actual sim speed", "time scale"], "Sync time + sim speed"),
        (["process_game_logic", "sim("], "Process simulation step"),
        (["selection", "follow view", "cursor"], "Update selection / camera"),
        (["add_type", "controls", "selection mode", "ui"], "Apply UI controls"),
        (["population manager", "population_mgr"], "Update population manager"),
        (["draw", "render", "window"], "Render frame"),
    ]

    for needles, label in groups:
        if any(needle in msg_lower for needle in needles):
            return label
    return None


def sequence_to_overview(lines: list[str]) -> list[str]:
    """
    Collapse the detailed sequence into a tiny ordered set of coarse actions.
    Control-flow blocks from the sequence are ignored; we only keep the mapped
    action buckets and render them once, grouped into init vs. main-loop steps.
    """
    seen_actions: set[str] = set()

    for raw in lines:
        line = raw.strip()
        if (
            not line
            or line.startswith("@startuml")
            or line.startswith("participant")
            or line.startswith("activate")
            or line.startswith("deactivate")
            or line.startswith("hide")
            or line.startswith("group")
            or line.startswith("alt ")
            or line.startswith("else")
            or line.startswith("loop")
            or line == "end"
        ):
            continue

        # Message lines contain ':' after '->'
        if "->" in line or "-->" in line:
            parts = line.split(":", 1)
            if len(parts) == 2:
                msg = clean_message(parts[1])
                mapped = map_action(msg)
                if mapped and not is_noise_action(msg):
                    seen_actions.add(mapped)
            continue

    out: list[str] = []
    out.append("@startuml")
    out.append("title Petri Dish Simulation")
    out.append("start")

    init_order = [
        "Initialize core systems",
        "Prepare initial spawns",
        "Initialize UI",
    ]
    loop_order = [
        "Sync time + sim speed",
        "Apply UI controls",
        "Process simulation step",
        "Update selection / camera",
        "Update population manager",
        "Render frame",
    ]

    for action in init_order:
        if action in seen_actions:
            out.append(f":{action};")

    loop_actions = [a for a in loop_order if a in seen_actions]
    if loop_actions:
        out.append("repeat")
        for action in loop_actions:
            out.append(f":{action};")
        out.append("repeat while (window.isOpen())")

    out.append("stop")
    out.append("@enduml")
    return out


def main() -> int:
    try:
        run(["clang-uml", "--config", "clang-uml.yml", "-n", SEQ_NAME, "-q"])
    except FileNotFoundError:
        print("clang-uml not found on PATH; install it first.", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as exc:
        print(f"clang-uml failed: {exc}", file=sys.stderr)
        return exc.returncode

    seq_lines = SEQ_PUML.read_text(encoding="utf-8").splitlines()
    activity_lines = sequence_to_overview(seq_lines)
    UML_DIR.mkdir(parents=True, exist_ok=True)
    ACT_PUML.write_text("\n".join(activity_lines) + "\n", encoding="utf-8")

    try:
        for fmt in ("svg", "png"):
            run(["plantuml", f"-t{fmt}", str(ACT_PUML)])
    except FileNotFoundError:
        print("plantuml not found on PATH; generated .puml but skipped rendering.", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as exc:
        print(f"plantuml failed: {exc}", file=sys.stderr)
        return exc.returncode

    print(f"Generated {ACT_PUML}, {ACT_SVG}, and {ACT_PNG}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
