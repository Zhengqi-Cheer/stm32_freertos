#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAKE_JOBS="${MAKE_JOBS:-$(nproc 2>/dev/null || echo 2)}"

usage() {
    cat <<EOF
Usage:
  ./build.sh [command] [make-options]

Commands:
  build       Build default App firmware
  boot        Build Stage0 + serial bootloader and a combined image
  stage0      Build Stage0 only
  boot_a      Build serial bootloader only
  app_a       Build App linked into App_A
  clean       Remove build output
  rebuild     Clean and build default App
  modules     Show module switches
  help        Show this help

Examples:
  ./build.sh
  ./build.sh boot
  ./build.sh app_a
  ./build.sh rebuild
  ./build.sh modules
EOF
}

combine_boot_image() {
    local stage0_bin="${SCRIPT_DIR}/build/stage0/stage0.bin"
    local boot_bin="${SCRIPT_DIR}/build/boot_a/boot_a.bin"
    local out_bin="${SCRIPT_DIR}/build/boot_combined.bin"
    local boot_offset=32768

    python3 - "${stage0_bin}" "${boot_bin}" "${out_bin}" "${boot_offset}" <<'PY'
import sys

stage0_path, boot_path, out_path, offset_s = sys.argv[1:]
offset = int(offset_s)

with open(stage0_path, "rb") as f:
    stage0 = f.read()
with open(boot_path, "rb") as f:
    boot = f.read()

if len(stage0) > offset:
    raise SystemExit("stage0.bin is larger than the Boot_A offset")

image = bytearray([0xFF]) * offset
image[0:len(stage0)] = stage0
image.extend(boot)

with open(out_path, "wb") as f:
    f.write(image)

print("wrote %s (%u bytes)" % (out_path, len(image)))
PY
}

cmd="${1:-build}"
if [[ $# -gt 0 ]]; then
    shift
fi

cd "${SCRIPT_DIR}"

case "${cmd}" in
    build)
        make -j"${MAKE_JOBS}" TARGET_IMAGE=app "$@"
        ;;
    boot)
        make -j"${MAKE_JOBS}" TARGET_IMAGE=stage0 "$@"
        make -j"${MAKE_JOBS}" TARGET_IMAGE=boot_a "$@"
        combine_boot_image
        ;;
    stage0)
        make -j"${MAKE_JOBS}" TARGET_IMAGE=stage0 "$@"
        ;;
    boot_a)
        make -j"${MAKE_JOBS}" TARGET_IMAGE=boot_a "$@"
        ;;
    app_a)
        make -j"${MAKE_JOBS}" TARGET_IMAGE=app_a "$@"
        ;;
    clean)
        make clean "$@"
        rm -rf "${SCRIPT_DIR}/build"
        ;;
    rebuild)
        make clean
        make -j"${MAKE_JOBS}" TARGET_IMAGE=app "$@"
        ;;
    modules)
        make list-modules "$@"
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "Unknown command: ${cmd}" >&2
        echo >&2
        usage >&2
        exit 1
        ;;
esac
