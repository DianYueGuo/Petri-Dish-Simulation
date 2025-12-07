#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
CONFIG="${CONFIG:-Debug}"

if [ ! -d "${BUILD_DIR}" ]; then
  cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${CONFIG}"
fi

cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target run "$@"
