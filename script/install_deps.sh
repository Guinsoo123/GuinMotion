#!/usr/bin/env bash
# Installs build tools and Qt 6 base by default (desktop GUI). Set GUINMOTION_SKIP_QT=1 to omit Qt only.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Non-empty and not "1" still installs Qt; only GUINMOTION_SKIP_QT=1 skips Qt packages.
install_qt_packages() {
  [[ "${GUINMOTION_SKIP_QT:-0}" != "1" ]]
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

brew_install_with_retry() {
  local max_attempts="${GUINMOTION_BREW_RETRY:-4}"
  local delay_seconds="${GUINMOTION_BREW_RETRY_DELAY:-10}"
  local attempt=1

  while [[ "${attempt}" -le "${max_attempts}" ]]; do
    if brew install "$@"; then
      return 0
    fi
    echo "brew install failed (attempt ${attempt}/${max_attempts})."
    echo "If you see curl (35) or ghcr.io errors, check network, VPN, proxy, or try a Homebrew mirror, then rerun."
    if [[ "${attempt}" -lt "${max_attempts}" ]]; then
      echo "Retrying in ${delay_seconds}s..."
      sleep "${delay_seconds}"
    fi
    attempt=$((attempt + 1))
  done
  return 1
}

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

install_macos() {
  if ! xcode-select -p >/dev/null 2>&1; then
    echo "Xcode Command Line Tools are required for the C++ compiler."
    echo "A macOS installer prompt will open. Finish the installation, then rerun this script."
    xcode-select --install || true
    exit 1
  fi

  if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is required on macOS. Install it from https://brew.sh and rerun this script."
    exit 1
  fi

  brew update
  if ! install_qt_packages; then
    brew_install_with_retry cmake ninja pkg-config
    echo "GUINMOTION_SKIP_QT=1: skipped Qt. Use CMake preset headless or GUINMOTION_ENABLE_QT=OFF for builds without GUI."
  else
    # Use qtbase only: enough for Qt6::Widgets and avoids pulling qtwebengine and other huge modules.
    brew_install_with_retry cmake ninja pkg-config qtbase
  fi
}

install_ubuntu() {
  if ! command -v apt-get >/dev/null 2>&1; then
    echo "This Linux installer currently supports Ubuntu/Debian apt-get environments."
    exit 1
  fi

  sudo apt-get update
  local base_pkgs=(
    build-essential
    cmake
    ninja-build
    pkg-config
    git
  )
  if ! install_qt_packages; then
    sudo apt-get install -y "${base_pkgs[@]}"
    echo "GUINMOTION_SKIP_QT=1: skipped Qt. Use CMake preset headless or GUINMOTION_ENABLE_QT=OFF for builds without GUI."
  else
    sudo apt-get install -y "${base_pkgs[@]}" qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
  fi
}

verify_common_tools() {
  ensure_command c++ "Install a C++ compiler. On macOS run xcode-select --install; on Ubuntu install build-essential."
  ensure_command cmake "Install CMake or rerun this script."

  if ! command -v ninja >/dev/null 2>&1 && ! command -v ninja-build >/dev/null 2>&1; then
    echo "Missing required command: ninja"
    echo "Install Ninja or rerun this script."
    exit 1
  fi
}

main() {
  echo "GuinMotion dependency installer"
  echo "Project: ${ROOT_DIR}"
  if install_qt_packages; then
    echo "Qt 6: will be installed (default). To skip Qt on a constrained machine, set GUINMOTION_SKIP_QT=1."
  else
    echo "Qt 6: skipped (GUINMOTION_SKIP_QT=1)."
  fi

  case "$(detect_os)" in
    macos) install_macos ;;
    ubuntu) install_ubuntu ;;
    linux)
      echo "Unsupported Linux distribution. This script currently installs packages for Ubuntu/Debian apt-get environments."
      echo "Install these manually: C++ compiler, cmake, ninja, pkg-config."
      exit 1
      ;;
    unsupported)
      echo "Unsupported OS: $(uname -s)"
      exit 1
      ;;
  esac

  verify_common_tools

  if install_qt_packages; then
    echo "Dependencies installed (default: Qt 6 base for the desktop GUI is included)."
  else
    echo "Dependencies installed (headless, no Qt packages)."
  fi
  echo "You can now run: ./script/build_and_run.sh"
  echo "Default behavior installs Qt. To skip Qt, run: GUINMOTION_SKIP_QT=1 ./script/install_deps.sh"
}

main "$@"
