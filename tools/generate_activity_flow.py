#!/usr/bin/env python3
"""
Generate an activity-style flow chart (with diamond branches) from the
clang-uml sequence diagram `petri_flow_simple`.

Pipeline:
 1) Run clang-uml to refresh uml/petri_flow_simple.puml
 2) Convert the sequence syntax (alt/else/loop/messages) into a PlantUML
    activity diagram with `if` diamonds.
 3) Render to SVG via plantuml.

Requires `clang-uml` and `plantuml` installed (both already used in this repo).
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
ACT_PUML = UML_DIR / "petri_flow_activity.puml"
ACT_SVG = ACT_PUML.with_suffix(".svg")
ACT_PNG = ACT_PUML.with_suffix(".png")


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True)


def clean_message(text: str) -> str:
    # Strip bold markers and bracket noise clang-uml adds around conditions.
    text = re.sub(r"[*\[\]]", "", text)
    text = text.strip()
    return text


def sequence_to_activity(lines: list[str]) -> list[str]:
    out: list[str] = []
    stack: list[str] = []

    out.append("@startuml")
    out.append("title Petri Dish Simulation - Auto Activity (from petri_flow_simple)")
    out.append("start")

    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("@startuml") or line.startswith("participant") or line.startswith("activate") or line.startswith("deactivate") or line.startswith("hide"):
            continue

        if line.startswith("group"):
            label = clean_message(line[5:])
            out.append(f"if ({label or 'group'}) then (yes)")
            stack.append("if")
            continue

        if line.startswith("alt "):
            cond = clean_message(line[4:])
            out.append(f"if ({cond or 'condition'}) then (yes)")
            stack.append("if")
            continue

        if line.startswith("else"):
            label = clean_message(line[4:])
            if label:
                out.append(f"elseif ({label})")
            else:
                out.append("else")
            continue

        if line.startswith("loop"):
            label = clean_message(line[5:])
            out.append("repeat")
            stack.append(f"loop::{label or 'loop'}")
            continue

        if line == "end":
            if stack:
                top = stack.pop()
                if top.startswith("loop::"):
                    cond = top.split("::", 1)[1]
                    out.append(f"repeat while ({cond})")
                else:
                    out.append("endif")
            continue

        # Message lines contain ':' after '->'
        if "->" in line or "-->" in line:
            parts = line.split(":", 1)
            if len(parts) == 2:
                msg = clean_message(parts[1])
                if msg:
                    out.append(f":{msg};")
            continue

    # Close any unmatched condition blocks.
    while stack:
        top = stack.pop()
        if top.startswith("loop::"):
            cond = top.split("::", 1)[1]
            out.append(f"repeat while ({cond})")
        else:
            out.append("endif")

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
    activity_lines = sequence_to_activity(seq_lines)
    UML_DIR.mkdir(parents=True, exist_ok=True)
    ACT_PUML.write_text("\n".join(activity_lines) + "\n", encoding="utf-8")

    try:
        for fmt in ("svg", "png"):
            run(["plantuml", f"-t{fmt}", str(ACT_PUML)])
    except FileNotFoundError:
        print("plantuml not found on PATH; generated .puml but skipped rendering.", file=sys.stderr)
        return 1

    print(f"Generated {ACT_PUML}, {ACT_SVG}, and {ACT_PNG}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
