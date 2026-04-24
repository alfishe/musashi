#!/usr/bin/env bash
# sst_runner.sh — Self-bootstrapping wrapper for sst_runner
#
# Checks that all prerequisites are in place before running tests:
#   1. sst_runner binary compiled  → triggers cmake build if missing
#   2. .sst vector files converted → triggers cmake (which runs converter) if missing
#   3. Raw test data cloned        → cmake clone step fires automatically if missing
#
# Usage is identical to the sst_runner binary — all arguments are forwarded:
#
#   ./test/singlestep/sst_runner.sh --all --summary
#   ./test/singlestep/sst_runner.sh --all --source=tomharte --summary
#   ./test/singlestep/sst_runner.sh --all --max-vectors=100 --verbose
#   ./test/singlestep/sst_runner.sh --help
#
# Can be called from any working directory — all paths are resolved relative
# to the script's own location.

set -euo pipefail

# ── Locate key paths ──────────────────────────────────────────────────────────
SST_DIR="$(cd "$(dirname "$0")" && pwd)"          # test/singlestep/
MUSASHI_ROOT="$(cd "$SST_DIR/../.." && pwd)"      # musashi root
BINARY="$SST_DIR/sst_runner"
BUILD_DIR="$SST_DIR/build"
UNIFIED_DIR="$SST_DIR/unified"
DATA_DIR="$SST_DIR/data"

# ── Helpers ───────────────────────────────────────────────────────────────────
info()  { printf '\033[1;34m[sst_runner.sh]\033[0m %s\n' "$*" >&2; }
warn()  { printf '\033[1;33m[sst_runner.sh]\033[0m %s\n' "$*" >&2; }
error() { printf '\033[1;31m[sst_runner.sh]\033[0m %s\n' "$*" >&2; exit 1; }

need_cmake_build=0

# ── Check 1: binary ───────────────────────────────────────────────────────────
if [[ ! -x "$BINARY" ]]; then
    warn "sst_runner binary not found: $BINARY"
    need_cmake_build=1
fi

# ── Check 2: converted .sst files ────────────────────────────────────────────
sst_count=0
if [[ -d "$UNIFIED_DIR" ]]; then
    sst_count=$(find "$UNIFIED_DIR" -name '*.sst' 2>/dev/null | wc -l | tr -d ' ')
fi
if [[ "$sst_count" -eq 0 ]]; then
    warn "No .sst files found in $UNIFIED_DIR"
    need_cmake_build=1
fi

# ── Run cmake if anything is missing ─────────────────────────────────────────
if [[ "$need_cmake_build" -eq 1 ]]; then
    if ! command -v cmake &>/dev/null; then
        # Detect platform and suggest the right install command
        OS="$(uname -s 2>/dev/null || echo unknown)"
        case "$OS" in
            Darwin)
                if command -v brew &>/dev/null; then
                    install_hint="  brew install cmake"
                else
                    install_hint="  # Install Homebrew first: https://brew.sh
  brew install cmake
  # — or install directly from https://cmake.org/download/"
                fi
                ;;
            Linux)
                if   command -v apt-get  &>/dev/null; then
                    install_hint="  sudo apt-get update && sudo apt-get install -y cmake gcc"
                elif command -v dnf      &>/dev/null; then
                    install_hint="  sudo dnf install -y cmake gcc"
                elif command -v yum      &>/dev/null; then
                    install_hint="  sudo yum install -y cmake gcc"
                elif command -v pacman   &>/dev/null; then
                    install_hint="  sudo pacman -S cmake gcc"
                elif command -v zypper   &>/dev/null; then
                    install_hint="  sudo zypper install cmake gcc"
                elif command -v apk      &>/dev/null; then
                    install_hint="  apk add cmake gcc musl-dev"
                else
                    install_hint="  # Install cmake 3.16+ from your distro's package manager
  # or: https://cmake.org/download/"
                fi
                ;;
            MINGW*|MSYS*|CYGWIN*)
                install_hint="  # Windows detected (Git Bash / MSYS2 / Cygwin)
  # Option 1 — winget:   winget install Kitware.CMake
  # Option 2 — choco:    choco install cmake
  # Option 3 — direct:   https://cmake.org/download/"
                ;;
            *)
                install_hint="  # https://cmake.org/download/"
                ;;
        esac

        error "cmake not found.

  cmake 3.16+ is required to auto-build the runner.

  Install it with:
$install_hint

  Alternatively, build manually:
    gcc -Wall -O2 -o '$BINARY' \\
      '$SST_DIR/sst_runner.c' '$SST_DIR/sst_loader.c' \\
      '$MUSASHI_ROOT/m68kcpu.o' '$MUSASHI_ROOT/m68kdasm.o' \\
      '$MUSASHI_ROOT/m68kops.o' '$MUSASHI_ROOT/softfloat/softfloat.o' \\
      -I'$MUSASHI_ROOT' -lm"
    fi

    info "Running cmake to prepare prerequisites..."
    info "  Build dir : $BUILD_DIR"
    info "  Root      : $MUSASHI_ROOT"

    cmake -B "$BUILD_DIR" "$SST_DIR" \
        -DMUSASHI_ROOT="$MUSASHI_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        --log-level=WARNING

    cmake --build "$BUILD_DIR" --parallel "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

    # Re-check after build
    [[ -x "$BINARY" ]] || error "cmake build succeeded but binary still missing: $BINARY"

    sst_count=$(find "$UNIFIED_DIR" -name '*.sst' 2>/dev/null | wc -l | tr -d ' ')
    [[ "$sst_count" -gt 0 ]] || error "cmake build succeeded but no .sst files found in $UNIFIED_DIR"

    info "Prerequisites ready — $sst_count .sst files, binary at $BINARY"
fi

# ── Execute the runner ────────────────────────────────────────────────────────
# If no arguments provided, default to --all --summary for a full regression run
ARGS=("$@")
if [[ ${#ARGS[@]} -eq 0 ]]; then
    info "No arguments provided. Running all tests by default..."
    ARGS=("--all" "--summary")
fi

# exec replaces this shell process, so signals/exit codes pass through cleanly.
# Working directory is always musashi root so relative paths in the binary work.
cd "$MUSASHI_ROOT"
exec "$BINARY" "${ARGS[@]}"
