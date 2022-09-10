add_executable(mytex-test)

# Add test sources
file(GLOB test_files "test_*.cpp")
target_sources(mytex-test PRIVATE
    ${test_files}
)

target_link_libraries(mytex-test PRIVATE baudvine-mytex)

set_target_properties(mytex-test
    PROPERTIES
    CXX_EXTENSIONS OFF
)

# Wire up googletest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

# googletest includes a default main() that's good enough here
target_link_libraries(mytex-test PRIVATE gtest_main gmock)
include(GoogleTest)
gtest_discover_tests(mytex-test)
