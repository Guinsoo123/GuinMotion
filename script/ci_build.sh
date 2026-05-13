#!/usr/bin/env bash
# CI-oriented build: configure + build + test using the headless preset (no Qt).
# Intended for macOS / Ubuntu where CMakePresets and Ninja are available.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${GUINMOTION_PRESET:-headless}"

cd "${ROOT_DIR}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake is required"
  exit 1
fi
if ! command -v ninja >/dev/null 2>&1 && ! command -v ninja-build >/dev/null 2>&1; then
  echo "ninja or ninja-build is required"
  exit 1
fi

echo "CI build: preset=${PRESET}"
cmake --preset "${PRESET}"
cmake --build --preset "${PRESET}"
ctest --test-dir "${ROOT_DIR}/build/${PRESET}" --output-on-failure

echo "CI build finished successfully."
