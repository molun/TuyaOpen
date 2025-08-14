#!/usr/bin/env bash

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREBUILDS_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROOT_DIR="$(cd "${PREBUILDS_DIR}/../.." && pwd)"

ELFUTILS_DIR="${PREBUILDS_DIR}/elfutils"
SIMAVR_DIR="${PREBUILDS_DIR}/simavr"

# Output directory to collect all headers and libraries
OUT_DIR="${PREBUILDS_DIR}/output"
OUT_INCLUDE_DIR="${OUT_DIR}/include"
OUT_LIB_DIR="${OUT_DIR}/lib"

# Optional toolchains (not strictly required for native builds)
# Provided path by user; not used for native simavr build, but exposed for convenience
ARM_GCC_ROOT_DEFAULT="/home/edward/TuyaOpen/platform/tools/gcc-arm-none-eabi-10.3-2021.10"
ARM_GCC_ROOT="${ARM_GCC_ROOT:-${ARM_GCC_ROOT_DEFAULT}}"

JOBS=${JOBS:-$(getconf _NPROCESSORS_ONLN || echo 4)}
VERBOSE=${V:-0}

usage() {
  echo "Usage: $0 [--clean] [--rebuild] [--out DIR]"
  echo "  --clean     Remove build artifacts and output directory"
  echo "  --rebuild   Clean first, then build"
  echo "  --out DIR   Output directory (default: ${OUT_DIR})"
}

CLEAN=0
REBUILD=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)
      CLEAN=1
      shift
      ;;
    --rebuild)
      REBUILD=1
      shift
      ;;
    --out)
      OUT_DIR="$2"; shift 2
      OUT_INCLUDE_DIR="${OUT_DIR}/include"
      OUT_LIB_DIR="${OUT_DIR}/lib"
      ;;
    -h|--help)
      usage; exit 0
      ;;
    *)
      echo "Unknown arg: $1"; usage; exit 1
      ;;
  esac
done

mkdir -p "${OUT_INCLUDE_DIR}" "${OUT_LIB_DIR}"

log() {
  echo "[prebuild] $*"
}

run_make() {
  if [[ "${VERBOSE}" -eq 1 ]]; then
    make -j"${JOBS}" "$@"
  else
    make -s -j"${JOBS}" "$@"
  fi
}

clean_all() {
  log "Cleaning output and build artifacts"
  rm -rf "${OUT_DIR}" || true
  # elfutils uses autotools; run distclean if possible
  if [[ -f "${ELFUTILS_DIR}/Makefile" ]]; then
    (cd "${ELFUTILS_DIR}" && run_make distclean || true)
  fi
  # simavr clean
  if [[ -f "${SIMAVR_DIR}/Makefile" ]]; then
    (cd "${SIMAVR_DIR}" && run_make clean || true)
  fi
}

if [[ "${CLEAN}" -eq 1 || "${REBUILD}" -eq 1 ]]; then
  clean_all
  [[ "${CLEAN}" -eq 1 && "${REBUILD}" -eq 0 ]] && exit 0
fi

# 1) Build libelf from elfutils (static)
build_libelf() {
  log "Configuring elfutils (for libelf only)"
  pushd "${ELFUTILS_DIR}" >/dev/null

  # Configure for native build; disable components we don't need
  # We build in-tree due to elfutils' build system simplicity
  if [[ ! -f Makefile ]]; then
    # Prefer clang/gcc available on host
    CC_FOR_BUILD=${CC:-gcc}
    CFLAGS_FOR_BUILD=${CFLAGS:-"-O2"}
    log "Running ./configure with CC=${CC_FOR_BUILD}"
    CC="${CC_FOR_BUILD}" CFLAGS="${CFLAGS_FOR_BUILD}" \
      ./configure \
        --disable-debuginfod \
        --disable-libdebuginfod \
        --disable-nls || { echo "elfutils configure failed"; exit 1; }
  fi

  log "Building libelf"
  run_make -C libelf libelf.a

  # Also build internal utility library used by libelf when linked statically
  run_make -C lib libeu.a

  log "Copying libelf artifacts to ${OUT_DIR}"
  install -d "${OUT_INCLUDE_DIR}" "${OUT_LIB_DIR}"
  install -m 0644 libelf/libelf.a "${OUT_LIB_DIR}/"
  install -m 0644 lib/libeu.a "${OUT_LIB_DIR}/" || true
  # Public headers
  install -m 0644 libelf/libelf.h "${OUT_INCLUDE_DIR}/"
  install -m 0644 libelf/gelf.h "${OUT_INCLUDE_DIR}/"
  install -m 0644 libelf/elf.h "${OUT_INCLUDE_DIR}/"

  popd >/dev/null
}

# 2) Build simavr against our libelf
build_simavr() {
  log "Building simavr (using libelf from ${OUT_DIR})"
  pushd "${SIMAVR_DIR}" >/dev/null

  # Pass include and lib paths via environment
  IPATH="${OUT_INCLUDE_DIR}" \
  LDFLAGS="-L${OUT_LIB_DIR} -lelf -leu -lz -lzstd ${LDFLAGS:-}" \
  CFLAGS="${CFLAGS:-} -Wno-error -Wno-stringop-truncation" \
  JOBS=1 run_make -C simavr config

  IPATH="${OUT_INCLUDE_DIR}" \
  LDFLAGS="-L${OUT_LIB_DIR} -lelf -leu -lz -lzstd ${LDFLAGS:-}" \
  CFLAGS="${CFLAGS:-} -Wno-error -Wno-stringop-truncation" \
  run_make -C simavr libsimavr

  # Collect artifacts
  # Static library is in simavr/obj-*/libsimavr.a
  OBJ_DIR=$(cd simavr && echo obj-*)
  if [[ -f "simavr/${OBJ_DIR}/libsimavr.a" ]]; then
    install -m 0644 "simavr/${OBJ_DIR}/libsimavr.a" "${OUT_LIB_DIR}/"
  else
    echo "libsimavr.a not found"; exit 1
  fi

  # Headers
  install -d "${OUT_INCLUDE_DIR}/simavr/avr" "${OUT_INCLUDE_DIR}/simavr/parts"
  install -m 0644 simavr/sim/*.h "${OUT_INCLUDE_DIR}/simavr/" || true
  # sim_core_*.h are generated during build
  if compgen -G "simavr/sim_core_*.h" > /dev/null; then
    install -m 0644 simavr/sim_core_*.h "${OUT_INCLUDE_DIR}/simavr/"
  fi
  install -m 0644 simavr/sim/avr/*.h "${OUT_INCLUDE_DIR}/simavr/avr/" || true
  install -m 0644 examples/parts/*.h "${OUT_INCLUDE_DIR}/simavr/parts/" || true

  popd >/dev/null
}

build_libelf
build_simavr

log "Done. Artifacts available in ${OUT_DIR}"


