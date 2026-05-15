#!/usr/bin/env bash
# Installs system build tools/Qt and bootstraps vcpkg for MuJoCo/Assimp/tinyxml2.
# Set GUINMOTION_SKIP_QT=1 to omit Qt only.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/script/env_common.sh"

install_qt_packages() {
  [[ "${GUINMOTION_SKIP_QT:-0}" != "1" ]]
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
    echo "If you see curl/ghcr.io errors, check network, VPN, proxy, or try a mirror, then rerun."
    if [[ "${attempt}" -lt "${max_attempts}" ]]; then
      echo "Retrying in ${delay_seconds}s..."
      sleep "${delay_seconds}"
    fi
    attempt=$((attempt + 1))
  done
  return 1
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
  # macOS ships tar with CLT; Homebrew has no "tar" formula (use gnu-tar only if GNU tar is required).
  if install_qt_packages; then
    brew_install_with_retry cmake ninja pkg-config git curl zip unzip qtbase
  else
    brew_install_with_retry cmake ninja pkg-config git curl zip unzip
    echo "GUINMOTION_SKIP_QT=1: skipped Qt. Use GUINMOTION_PRESET=headless for builds without GUI."
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
    curl
    zip
    unzip
    tar
    autoconf
    automake
    autoconf-archive
    libtool
  )
  if install_qt_packages; then
    sudo apt-get install -y "${base_pkgs[@]}" qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
  else
    sudo apt-get install -y "${base_pkgs[@]}"
    echo "GUINMOTION_SKIP_QT=1: skipped Qt. Use GUINMOTION_PRESET=headless for builds without GUI."
  fi
}

vcpkg_triplet() {
  case "$(uname -s)" in
    Darwin)
      case "$(uname -m)" in
        arm64) echo "arm64-osx" ;;
        *) echo "x64-osx" ;;
      esac
      ;;
    Linux) echo "x64-linux" ;;
    *) echo "" ;;
  esac
}

bootstrap_vcpkg() {
  local vcpkg_root
  vcpkg_root="$(resolve_vcpkg_root "${ROOT_DIR}")"

  if [[ ! -d "${vcpkg_root}/.git" ]]; then
    echo "Cloning vcpkg into: ${vcpkg_root}"
    mkdir -p "$(dirname "${vcpkg_root}")"
    git clone https://github.com/microsoft/vcpkg.git "${vcpkg_root}"
  else
    echo "Using existing vcpkg checkout: ${vcpkg_root}"
  fi

  if [[ ! -x "${vcpkg_root}/vcpkg" ]]; then
    echo "Bootstrapping vcpkg"
    "${vcpkg_root}/bootstrap-vcpkg.sh" -disableMetrics
  fi
}

install_vcpkg_manifest_deps() {
  local vcpkg_root
  vcpkg_root="$(resolve_vcpkg_root "${ROOT_DIR}")"
  local triplet
  triplet="${VCPKG_DEFAULT_TRIPLET:-$(vcpkg_triplet)}"
  if [[ -z "${triplet}" ]]; then
    echo "Could not infer vcpkg triplet. Set VCPKG_DEFAULT_TRIPLET manually."
    exit 1
  fi

  echo "Installing vcpkg manifest dependencies for triplet: ${triplet}"
  echo "This installs MuJoCo, Assimp and tinyxml2 from vcpkg.json."
  export VCPKG_FEATURE_FLAGS="${VCPKG_FEATURE_FLAGS:-manifests}"
  (cd "${ROOT_DIR}" && "${vcpkg_root}/vcpkg" install --triplet "${triplet}" --clean-after-build)
}

main() {
  echo "GuinMotion dependency installer"
  echo "Project: ${ROOT_DIR}"
  if install_qt_packages; then
    echo "Qt 6: will be installed (default). To skip Qt, set GUINMOTION_SKIP_QT=1."
  else
    echo "Qt 6: skipped (GUINMOTION_SKIP_QT=1)."
  fi

  local os_name
  os_name="$(detect_os)"
  verify_supported_os "${os_name}"
  case "${os_name}" in
    macos) install_macos ;;
    ubuntu) install_ubuntu ;;
  esac

  verify_build_tools
  ensure_command git "Install git or rerun this script."
  ensure_command curl "Install curl or rerun this script."

  bootstrap_vcpkg
  install_vcpkg_manifest_deps

  echo "Dependencies installed."
  echo "You can now run: ./script/build_and_run.sh"
  echo "For CLI-only environments: GUINMOTION_PRESET=headless ./script/build_and_run.sh"
}

main "$@"
