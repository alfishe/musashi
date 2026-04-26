include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Musashi GTests ────────────────────────────────────────────────────────────
file(GLOB_RECURSE MUSASHI_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/instructions/**/*.cpp"
)

add_executable(musashi_unittests
    gtest_main.cpp
    framework/m68k_fixture.cpp
    ${MUSASHI_TEST_SOURCES}
)

target_link_libraries(musashi_unittests
    PRIVATE
        musashi_core
        GTest::gtest
        GTest::gtest_main
)

# Add includes for framework
target_include_directories(musashi_unittests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/framework)

# Enable discoverable tests
include(GoogleTest)
gtest_discover_tests(musashi_unittests)
