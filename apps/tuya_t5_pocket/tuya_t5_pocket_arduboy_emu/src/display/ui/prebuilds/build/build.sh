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
  echo "Usage: $0 [--clean] [--rebuild] [--out DIR] [--arm-gcc PATH]"
  echo "  --clean     Remove build artifacts and output directory"
  echo "  --rebuild   Clean first, then build"
  echo "  --out DIR   Output directory (default: ${OUT_DIR})"
  echo "  --arm-gcc PATH  ARM GCC toolchain path (default: ${ARM_GCC_ROOT})"
  echo ""
  echo "Note: This build script forces ARM bare metal cross-compilation"
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
    --arm-gcc)
      ARM_GCC_ROOT="$2"; shift 2
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

  # libelf must be built natively (can't use bare metal toolchain)
  # but simavr will use ARM bare metal toolchain
  log "Building libelf with native compiler (required for cross-compilation)"
  CC_FOR_BUILD=${CC:-gcc}
  CFLAGS_FOR_BUILD=${CFLAGS:-"-O2"}
  HOST_FLAG=""

  # Configure for build; disable components we don't need
  # We build in-tree due to elfutils' build system simplicity
  if [[ ! -f Makefile ]]; then
    log "Running ./configure with CC=${CC_FOR_BUILD}"
    CC="${CC_FOR_BUILD}" CFLAGS="${CFLAGS_FOR_BUILD}" \
      ./configure ${HOST_FLAG} \
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

  # Force ARM bare metal toolchain for simavr
  if [[ ! -f "${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc" ]]; then
    echo "Error: ARM cross-compiler not found at ${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc"
    exit 1
  fi
  log "Using ARM bare metal cross-compiler: ${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc"
  log "Note: This will build simavr for ARM bare metal targets"
  CC="${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc"
  AR="${ARM_GCC_ROOT}/bin/arm-none-eabi-ar"
  RANLIB="${ARM_GCC_ROOT}/bin/arm-none-eabi-ranlib"
  # Add flags for bare metal compilation with POSIX support
  CFLAGS="${CFLAGS:-} -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -Wno-error=format -Wno-error=char-subscripts -Wno-format -Wno-char-subscripts"

  # Pass include and lib paths via environment
  IPATH="${OUT_INCLUDE_DIR}" \
  LDFLAGS="-L${OUT_LIB_DIR} -lelf -leu -lz -lzstd ${LDFLAGS:-}" \
  CFLAGS="${CFLAGS:-} -Wno-error -Wno-stringop-truncation" \
  CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" \
  JOBS=1 run_make -C simavr config

  IPATH="${OUT_INCLUDE_DIR}" \
  LDFLAGS="-L${OUT_LIB_DIR} -lelf -leu -lz -lzstd ${LDFLAGS:-}" \
  CFLAGS="${CFLAGS:-} -Wno-error -Wno-stringop-truncation" \
  CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" \
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


