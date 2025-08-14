# Prebuilds Orchestrator

This folder contains a script that prebuilds `libelf` from `elfutils/` and then builds `simavr/` against that `libelf`, collecting headers and libraries into one output directory.

## Prereqs
- Linux host with a native C toolchain (gcc or clang)
- `make`, `bash`
- Optional (not used for native build): `/home/edward/TuyaOpen/platform/tools/gcc-arm-none-eabi-10.3-2021.10`

## Usage

- Clean only:
```bash
bash build.sh --clean
```

- Rebuild from scratch into `../output`:
```bash
bash build.sh --rebuild
```

- Custom output directory:
```bash
bash build.sh --out /abs/path/to/output
```

Artifacts in `output/`:
- `include/`: public headers for `libelf` and `simavr`
- `lib/`: `libelf.a`, `libsimavr.a`

Notes:
- The script avoids re-configuring `elfutils` when `Makefile` exists.
- `simavr` is pointed at `output/include` and `output/lib` via `IPATH` and `LDFLAGS`.

