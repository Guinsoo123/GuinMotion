#!/usr/bin/env bash

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
  if brew --prefix qtbase &>/dev/null; then
    qt_root="$(brew --prefix qtbase)"
  elif brew --prefix qt &>/dev/null; then
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

resolve_vcpkg_root() {
  local root_dir="$1"
  if [[ -n "${VCPKG_ROOT:-}" ]]; then
    echo "${VCPKG_ROOT}"
  else
    echo "${root_dir}/.deps/vcpkg"
  fi
}

setup_vcpkg_toolchain() {
  local root_dir="$1"
  local vcpkg_root
  vcpkg_root="$(resolve_vcpkg_root "${root_dir}")"
  local toolchain="${vcpkg_root}/scripts/buildsystems/vcpkg.cmake"

  if [[ ! -f "${toolchain}" ]]; then
    echo "vcpkg toolchain not found: ${toolchain}"
    echo "Run ./script/install_deps.sh first, or set VCPKG_ROOT to an existing vcpkg checkout."
    exit 1
  fi

  export CMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE:-${toolchain}}"
  export VCPKG_FEATURE_FLAGS="${VCPKG_FEATURE_FLAGS:-manifests}"
  export VCPKG_DEFAULT_TRIPLET="${VCPKG_DEFAULT_TRIPLET:-$(vcpkg_default_triplet)}"
  echo "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}"
  echo "Using vcpkg triplet: ${VCPKG_DEFAULT_TRIPLET}"
}

vcpkg_default_triplet() {
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

verify_supported_os() {
  local os_name="$1"
  if [[ "${os_name}" == "unsupported" ]]; then
    echo "Unsupported OS: $(uname -s). This script supports macOS and Ubuntu/Debian."
    exit 1
  fi
  if [[ "${os_name}" == "linux" ]]; then
    echo "Unsupported Linux distribution for this script. Use Ubuntu/Debian or set paths manually."
    exit 1
  fi
}

reset_build_dir_if_toolchain_changed() {
  local root_dir="$1"
  local preset_name="$2"
  local build_dir="${root_dir}/build/${preset_name}"
  local cache_file="${build_dir}/CMakeCache.txt"

  if [[ ! -f "${cache_file}" ]]; then
    return 0
  fi

  if grep -Fq "CMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}" "${cache_file}" ||
     grep -Fq "CMAKE_TOOLCHAIN_FILE:UNINITIALIZED=${CMAKE_TOOLCHAIN_FILE}" "${cache_file}"; then
    return 0
  fi

  echo "Build directory was configured with a different/no CMake toolchain."
  echo "Removing generated cache so vcpkg/MuJoCo dependencies can be configured cleanly: ${build_dir}"
  rm -rf "${build_dir}"
}
