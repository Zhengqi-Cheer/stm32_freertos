#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAKE_JOBS="${MAKE_JOBS:-$(nproc 2>/dev/null || echo 2)}"

usage() {
    cat <<EOF
Usage:
  ./build.sh [command] [make-options]

Commands:
  build       Build firmware, default command
  clean       Remove build output
  rebuild     Clean and build firmware
  modules     Show module switches
  partition   Generate partition config files
  help        Show this help

Examples:
  ./build.sh
  ./build.sh rebuild
  ./build.sh modules
  ./build.sh partition
  ./build.sh build ENABLE_CORE_LED=n
  MAKE_JOBS=4 ./build.sh
EOF
}

cmd="${1:-build}"
if [[ $# -gt 0 ]]; then
    shift
fi

cd "${SCRIPT_DIR}"

case "${cmd}" in
    build)
        make -j"${MAKE_JOBS}" "$@"
        ;;
    clean)
        make clean "$@"
        ;;
    rebuild)
        make clean
        make -j"${MAKE_JOBS}" "$@"
        ;;
    modules)
        make list-modules "$@"
        ;;
    partition)
        make partition "$@"
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
