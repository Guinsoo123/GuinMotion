#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${GUINMOTION_PRESET:-default}"

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

verify_build_tools() {
  ensure_command cmake "Run ./script/install_deps.sh first."
  ensure_command c++ "Run ./script/install_deps.sh first."

  if ! command -v ninja >/dev/null 2>&1 && ! command -v ninja-build >/dev/null 2>&1; then
    echo "Missing required command: ninja"
    echo "Run ./script/install_deps.sh first."
    exit 1
  fi
}

# Homebrew Qt is not always on CMake's default search path on macOS.
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
verify_build_tools
setup_qt_cmake_prefix "${OS_NAME}" "${PRESET}"

echo "Configuring GuinMotion with preset: ${PRESET}"
cmake --preset "${PRESET}"

echo "Building GuinMotion"
cmake --build --preset "${PRESET}"

echo "Running CTest"
ctest --test-dir "${ROOT_DIR}/build/${PRESET}" --output-on-failure

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
