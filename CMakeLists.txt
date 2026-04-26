cmake_minimum_required(VERSION 3.16)
project(musashi C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Musashi Core ──────────────────────────────────────────────────────────────
set(MUSASHI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SOFTFLOAT_DIR ${MUSASHI_ROOT}/softfloat)

# m68kmake code generator
add_executable(m68kmake EXCLUDE_FROM_ALL "${MUSASHI_ROOT}/m68kmake.c")

# Generate m68kops.c / m68kops.h
add_custom_command(
    OUTPUT  "${MUSASHI_ROOT}/m68kops.c" "${MUSASHI_ROOT}/m68kops.h"
    COMMAND m68kmake
    WORKING_DIRECTORY "${MUSASHI_ROOT}"
    DEPENDS m68kmake "${MUSASHI_ROOT}/m68k_in.c"
    COMMENT "Generating m68kops.c / m68kops.h"
)

set_source_files_properties("${MUSASHI_ROOT}/m68kops.c" PROPERTIES GENERATED TRUE)

add_library(musashi_core STATIC
    "${MUSASHI_ROOT}/m68kcpu.c"
    "${MUSASHI_ROOT}/m68kdasm.c"
    "${MUSASHI_ROOT}/m68kops.c"
    "${SOFTFLOAT_DIR}/softfloat.c"
)
target_include_directories(musashi_core PUBLIC "${MUSASHI_ROOT}")
target_compile_options(musashi_core PRIVATE -O2 -Wall)

# ── Subdirectories ───────────────────────────────────────────────────────────
add_subdirectory(test/gtest)
# Note: test/singlestep/ has its own standalone CMakeLists.txt that 
# duplicates some of this, but for now we'll keep it separate or 
# integrate it later if requested.

