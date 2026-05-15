#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${GUINMOTION_PRESET:-release}"
source "${ROOT_DIR}/script/env_common.sh"

print_expected_outputs() {
  local os_name="$1"

  case "${os_name}" in
    macos)
      echo "macOS package generator: DragNDrop (.dmg) and TGZ, as configured by CPack."
      ;;
    ubuntu)
      echo "Ubuntu/Linux package generator: TGZ by default. AppImage can be added later."
      ;;
  esac
}

cd "${ROOT_DIR}"

OS_NAME="$(detect_os)"
verify_supported_os "${OS_NAME}"

echo "Detected platform: ${OS_NAME}"
verify_build_tools
setup_qt_cmake_prefix "${OS_NAME}" "${PRESET}"
setup_vcpkg_toolchain "${ROOT_DIR}"
reset_build_dir_if_toolchain_changed "${ROOT_DIR}" "${PRESET}"
print_expected_outputs "${OS_NAME}"

echo "Configuring release build with preset: ${PRESET}"
cmake --preset "${PRESET}" \
  -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
  -DVCPKG_TARGET_TRIPLET="${VCPKG_DEFAULT_TRIPLET}" \
  -DGUINMOTION_REQUIRE_ROBOTICS_DEPS=ON

echo "Building release artifacts"
cmake --build --preset "${PRESET}"

echo "Running CTest"
ctest --test-dir "${ROOT_DIR}/build/${PRESET}" --output-on-failure

echo "Running one-stop MuJoCo validation example"
"${ROOT_DIR}/build/${PRESET}/bin/guinmotion_one_stop_validation"

BUILD_DIR="${ROOT_DIR}/build/${PRESET}"

echo "Creating CPack package"
cmake --build "${BUILD_DIR}" --target package

echo "Package outputs:"
if command -v rg >/dev/null 2>&1; then
  rg --files "${BUILD_DIR}" | rg 'GuinMotion.*\.(dmg|tar\.gz|tgz|zip|AppImage|deb)$' || true
else
  find "${BUILD_DIR}" -maxdepth 2 -type f \( -name '*.dmg' -o -name '*.tar.gz' -o -name '*.tgz' -o -name '*.zip' -o -name '*.AppImage' -o -name '*.deb' \)
fi
