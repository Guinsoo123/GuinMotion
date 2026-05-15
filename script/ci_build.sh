#!/usr/bin/env bash
# CI-oriented build: configure + build + test using the headless preset (no Qt).
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${GUINMOTION_PRESET:-headless}"
source "${ROOT_DIR}/script/env_common.sh"

cd "${ROOT_DIR}"

verify_build_tools
setup_vcpkg_toolchain "${ROOT_DIR}"
reset_build_dir_if_toolchain_changed "${ROOT_DIR}" "${PRESET}"

echo "CI build: preset=${PRESET}"
cmake --preset "${PRESET}" \
  -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
  -DVCPKG_TARGET_TRIPLET="${VCPKG_DEFAULT_TRIPLET}" \
  -DGUINMOTION_REQUIRE_ROBOTICS_DEPS=ON
cmake --build --preset "${PRESET}"
ctest --test-dir "${ROOT_DIR}/build/${PRESET}" --output-on-failure

echo "CI build finished successfully."
