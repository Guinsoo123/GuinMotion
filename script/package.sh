#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${GUINMOTION_PRESET:-release}"

detect_os() {
  case "$(uname -s)" in
    Darwin) echo "macos" ;;
    Linux)
      if [[ -r /etc/os-release ]] && grep -qiE 'ubuntu|debian' /etc/os-release; then
        echo "ubuntu"
      else
        echo "linux"
      fi
      ;;
    *) echo "unsupported" ;;
  esac
}

ensure_command() {
  local command_name="$1"
  local hint="$2"

  if ! command -v "${command_name}" >/dev/null 2>&1; then
    echo "Missing required command: ${command_name}"
    echo "${hint}"
    exit 1
  fi
}

verify_package_tools() {
  ensure_command cmake "Run ./script/install_deps.sh first."
  ensure_command c++ "Run ./script/install_deps.sh first."

  if ! command -v ninja >/dev/null 2>&1 && ! command -v ninja-build >/dev/null 2>&1; then
    echo "Missing required command: ninja"
    echo "Run ./script/install_deps.sh first."
    exit 1
  fi
}

print_expected_outputs() {
  local os_name="$1"

  case "${os_name}" in
    macos)
      echo "macOS package generator: DragNDrop (.dmg) and TGZ, as configured by CPack."
      ;;
    ubuntu)
      echo "Ubuntu/Linux package generator: TGZ by default. AppImage can be added later when the AppImage toolchain is introduced."
      ;;
  esac
}

setup_qt_cmake_prefix() {
  local os_name="$1"
  local preset_name="$2"

  if [[ "${preset_name}" == "headless" ]]; then
    return 0
  fi

  if [[ "${os_name}" != "macos" ]]; then
    return 0
  fi

  if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew not found. Install Qt with ./script/install_deps.sh or set CMAKE_PREFIX_PATH manually."
    return 0
  fi

  local qt_root=""
  if brew --prefix qt &>/dev/null; then
    qt_root="$(brew --prefix qt)"
  elif brew --prefix qt@6 &>/dev/null; then
    qt_root="$(brew --prefix qt@6)"
  fi

  if [[ -z "${qt_root}" ]]; then
    echo "Could not resolve Qt prefix via brew. Install Qt: ./script/install_deps.sh"
    exit 1
  fi

  export CMAKE_PREFIX_PATH="${qt_root}${CMAKE_PREFIX_PATH:+;${CMAKE_PREFIX_PATH}}"
  echo "Using Qt from: ${qt_root}"
}

cd "${ROOT_DIR}"

OS_NAME="$(detect_os)"
if [[ "${OS_NAME}" == "unsupported" ]]; then
  echo "Unsupported OS: $(uname -s). This script supports macOS and Ubuntu/Debian."
  exit 1
fi
if [[ "${OS_NAME}" == "linux" ]]; then
  echo "Unsupported Linux distribution for this script. Use Ubuntu/Debian or set paths manually."
  exit 1
fi

echo "Detected platform: ${OS_NAME}"
verify_package_tools
setup_qt_cmake_prefix "${OS_NAME}" "${PRESET}"
print_expected_outputs "${OS_NAME}"

echo "Configuring release build with preset: ${PRESET}"
cmake --preset "${PRESET}"

echo "Building release artifacts"
cmake --build --preset "${PRESET}"

BUILD_DIR="${ROOT_DIR}/build/${PRESET}"

echo "Creating CPack package"
cmake --build "${BUILD_DIR}" --target package

echo "Package outputs:"
if command -v rg >/dev/null 2>&1; then
  rg --files "${BUILD_DIR}" | rg 'GuinMotion.*\.(dmg|tar\.gz|tgz|zip|AppImage|deb)$' || true
else
  find "${BUILD_DIR}" -maxdepth 2 -type f \( -name '*.dmg' -o -name '*.tar.gz' -o -name '*.tgz' -o -name '*.zip' -o -name '*.AppImage' -o -name '*.deb' \)
fi
