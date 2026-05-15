#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${GUINMOTION_PRESET:-default}"
source "${ROOT_DIR}/script/env_common.sh"

cd "${ROOT_DIR}"

OS_NAME="$(detect_os)"
verify_supported_os "${OS_NAME}"

echo "Detected platform: ${OS_NAME}"
verify_build_tools
setup_qt_cmake_prefix "${OS_NAME}" "${PRESET}"
setup_vcpkg_toolchain "${ROOT_DIR}"
reset_build_dir_if_toolchain_changed "${ROOT_DIR}" "${PRESET}"

echo "Configuring GuinMotion with preset: ${PRESET}"
cmake --preset "${PRESET}" \
  -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
  -DVCPKG_TARGET_TRIPLET="${VCPKG_DEFAULT_TRIPLET}" \
  -DGUINMOTION_REQUIRE_ROBOTICS_DEPS=ON

echo "Building GuinMotion"
cmake --build --preset "${PRESET}"

echo "Running CTest"
ctest --test-dir "${ROOT_DIR}/build/${PRESET}" --output-on-failure

echo "Running one-stop MuJoCo validation example"
"${ROOT_DIR}/build/${PRESET}/bin/guinmotion_one_stop_validation"

APP_PATH="${ROOT_DIR}/build/${PRESET}/bin/GuinMotion"

if [[ ! -x "${APP_PATH}" ]]; then
  echo "Executable not found: ${APP_PATH}"
  exit 1
fi

if [[ "${PRESET}" == "headless" ]]; then
  echo "Running CLI build: ${APP_PATH}"
  exec "${APP_PATH}" "$@"
fi

echo "Launching GuinMotion GUI (close the window to end the process)..."
exec "${APP_PATH}" "$@"
