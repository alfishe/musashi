# CMakeLists.txt — SingleStepTests Verification Harness
#
# Prep-only: clones test data repos, converts vectors to .sst, builds sst_runner.
# The sst_runner binary is always placed in test/singlestep/ regardless of where
# cmake is invoked from, so manual test runs are always via the same path.
# The canonical build directory is test/singlestep/build/ to keep everything
# self-contained and avoid polluting the musashi project root.
#
# Usage from musashi root:
#   cmake -B test/singlestep/build test/singlestep/
#   cmake --build test/singlestep/build
#
# Usage from test/singlestep/ directly:
#   cmake -B build .
#   cmake --build build
#
# With explicit root override (for unusual invocation locations):
#   cmake -B test/singlestep/build test/singlestep/ -DMUSASHI_ROOT=/path/to/musashi
#
# After build, run tests manually from the musashi root:
#   ./test/singlestep/sst_runner --all --summary
#   ./test/singlestep/sst_runner --all --source=tomharte --summary
#   ./test/singlestep/sst_runner --all --source=raddad --summary

cmake_minimum_required(VERSION 3.16)

# ── Standalone vs sub-project detection ──────────────────────────────────────
if(NOT DEFINED MUSASHI_ROOT)
    # Try to auto-detect: walk up from this file's dir
    get_filename_component(_sst_dir "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    get_filename_component(_test_dir "${_sst_dir}" DIRECTORY)
    get_filename_component(_root_dir "${_test_dir}" DIRECTORY)
    if(EXISTS "${_root_dir}/m68kcpu.c")
        set(MUSASHI_ROOT "${_root_dir}")
    else()
        message(FATAL_ERROR
            "Cannot locate Musashi root. Set -DMUSASHI_ROOT=/path/to/musashi")
    endif()
endif()

if(NOT PROJECT_NAME)
    project(musashi_sst_harness C)
endif()

# ── Paths ─────────────────────────────────────────────────────────────────────
set(SST_DIR      "${CMAKE_CURRENT_LIST_DIR}")
set(DATA_DIR     "${SST_DIR}/data")
set(UNIFIED_DIR  "${SST_DIR}/unified")
set(REPORTS_DIR  "${SST_DIR}/reports")
set(TOOLS_DIR    "${MUSASHI_ROOT}/tools")
set(SOFTFLOAT_DIR "${MUSASHI_ROOT}/softfloat")

# ── Python ────────────────────────────────────────────────────────────────────
find_package(Python3 REQUIRED COMPONENTS Interpreter)
message(STATUS "Python3: ${Python3_EXECUTABLE}")

# ── Git ───────────────────────────────────────────────────────────────────────
find_package(Git REQUIRED)

# ── m68kmake code generator ───────────────────────────────────────────────────
# Needed to produce m68kops.c / m68kops.h if not already present
add_executable(m68kmake EXCLUDE_FROM_ALL "${MUSASHI_ROOT}/m68kmake.c")

# ── Generate m68kops.c / m68kops.h ───────────────────────────────────────────
add_custom_command(
    OUTPUT  "${MUSASHI_ROOT}/m68kops.c" "${MUSASHI_ROOT}/m68kops.h"
    COMMAND m68kmake
    WORKING_DIRECTORY "${MUSASHI_ROOT}"
    DEPENDS m68kmake "${MUSASHI_ROOT}/m68k_in.c"
    COMMENT "Generating m68kops.c / m68kops.h"
)

# ── Musashi core static library ───────────────────────────────────────────────
add_library(musashi_core STATIC
    "${MUSASHI_ROOT}/m68kcpu.c"
    "${MUSASHI_ROOT}/m68kdasm.c"
    "${MUSASHI_ROOT}/m68kops.c"
    "${SOFTFLOAT_DIR}/softfloat.c"
)
target_include_directories(musashi_core PUBLIC "${MUSASHI_ROOT}")
target_compile_options(musashi_core PRIVATE -O2 -Wall)
# Enable address error + prefetch emulation for SST tests — real 68000 has a
# 2-word prefetch queue and generates address errors on odd-address word/long
# accesses.  The default m68kconf.h has both OFF for co-simulation use; we
# override them here via the #ifndef guards.
target_compile_definitions(musashi_core PUBLIC
    M68K_EMULATE_ADDRESS_ERROR=1
    M68K_EMULATE_PREFETCH=1
)
# m68kops.c is generated; ensure the generator runs first
set_source_files_properties(
    "${MUSASHI_ROOT}/m68kops.c" PROPERTIES GENERATED TRUE
)

# ── sst_runner executable ─────────────────────────────────────────────────────
add_executable(sst_runner
    "${SST_DIR}/sst_runner.c"
    "${SST_DIR}/sst_loader.c"
)
target_include_directories(sst_runner PRIVATE "${MUSASHI_ROOT}")
target_link_libraries(sst_runner PRIVATE musashi_core m)
target_compile_options(sst_runner PRIVATE -O2 -Wall -Wextra)
# Place binary in test/singlestep/ so it's usable without knowing the build dir
set_target_properties(sst_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SST_DIR}"
)

# ── Step 1: Clone test data repos if missing ──────────────────────────────────
set(TOMHARTE_REPO "https://github.com/SingleStepTests/680x0.git")
set(RADDAD_REPO   "https://github.com/SingleStepTests/m68000.git")
set(TOMHARTE_DIR  "${DATA_DIR}/680x0")
set(RADDAD_DIR    "${DATA_DIR}/m68000")

# Stamp files track whether clone has been done
set(TOMHARTE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/tomharte_clone.stamp")
set(RADDAD_STAMP  "${CMAKE_CURRENT_BINARY_DIR}/raddad_clone.stamp")

add_custom_command(
    OUTPUT  "${TOMHARTE_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DATA_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "-- Cloning TomHarte/680x0 test vectors..."
    COMMAND ${GIT_EXECUTABLE} clone --depth=1 "${TOMHARTE_REPO}" "${TOMHARTE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${TOMHARTE_STAMP}"
    COMMENT "Cloning SingleStepTests/680x0 (TomHarte) — one-time setup"
    VERBATIM
)

add_custom_command(
    OUTPUT  "${RADDAD_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DATA_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "-- Cloning raddad772/m68000 test vectors..."
    COMMAND ${GIT_EXECUTABLE} clone --depth=1 "${RADDAD_REPO}" "${RADDAD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${RADDAD_STAMP}"
    COMMENT "Cloning SingleStepTests/m68000 (raddad772) — one-time setup"
    VERBATIM
)

# Skip clone if dir already exists (resume-safe)
if(EXISTS "${TOMHARTE_DIR}/.git")
    file(TOUCH "${TOMHARTE_STAMP}")
endif()
if(EXISTS "${RADDAD_DIR}/.git")
    file(TOUCH "${RADDAD_STAMP}")
endif()

add_custom_target(sst_fetch_data
    DEPENDS "${TOMHARTE_STAMP}" "${RADDAD_STAMP}"
    COMMENT "Test data repos present"
)

# ── Step 2: Convert vectors to .sst ──────────────────────────────────────────
set(CONVERT_STAMP "${CMAKE_CURRENT_BINARY_DIR}/sst_convert.stamp")

add_custom_command(
    OUTPUT  "${CONVERT_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${UNIFIED_DIR}/tomharte"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${UNIFIED_DIR}/raddad"
    COMMAND ${Python3_EXECUTABLE} "${TOOLS_DIR}/sst_convert.py"
            --source=both
            --tomharte-dir=${TOMHARTE_DIR}/68000/v1
            --raddad-dir=${RADDAD_DIR}/v1
            --output-dir=${UNIFIED_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch "${CONVERT_STAMP}"
    DEPENDS "${TOMHARTE_STAMP}" "${RADDAD_STAMP}" "${TOOLS_DIR}/sst_convert.py"
    COMMENT "Converting test vectors to unified .sst format"
    VERBATIM
)

# Skip conversion if unified dir already has .sst files
file(GLOB _existing_sst "${UNIFIED_DIR}/tomharte/*.sst")
if(_existing_sst)
    file(TOUCH "${CONVERT_STAMP}")
endif()

add_custom_target(sst_convert
    DEPENDS "${CONVERT_STAMP}"
    COMMENT "Test vectors converted"
)

# ── Step 3: Default build target wires everything together ────────────────────
# cmake --build <dir>  →  sst_fetch_data + sst_convert + sst_runner
add_custom_target(sst_prep ALL
    DEPENDS sst_runner sst_fetch_data sst_convert
    COMMENT "SST prep complete — run test/singlestep/sst_runner manually"
)

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "── SST Harness ──────────────────────────────────")
message(STATUS "  Musashi root : ${MUSASHI_ROOT}")
message(STATUS "  Data dir     : ${DATA_DIR}")
message(STATUS "  Unified .sst : ${UNIFIED_DIR}")
message(STATUS "  Reports      : ${REPORTS_DIR}")
message(STATUS "  Binary       : ${SST_DIR}/sst_runner")
message(STATUS "")
message(STATUS "  After build, run tests manually:")
message(STATUS "    ./test/singlestep/sst_runner --all --summary")
message(STATUS "    ./test/singlestep/sst_runner --all --source=tomharte --summary")
message(STATUS "    ./test/singlestep/sst_runner --all --source=raddad  --summary")
message(STATUS "─────────────────────────────────────────────────")

