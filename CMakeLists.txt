cmake_minimum_required(VERSION 3.15)
project(mytex)

option(mytex_TESTS "Build Mytex tests" ON)

add_library(baudvine-mytex INTERFACE)

target_include_directories(baudvine-mytex INTERFACE include)
target_compile_features(baudvine-mytex INTERFACE cxx_std_11)

if (MSVC)
    add_compile_options(
            /W4
    )
else ()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif ()

# Install configuration
set_target_properties(baudvine-mytex
        PROPERTIES
        PUBLIC_HEADER include/baudvine/mytex.h
        )
include(GNUInstallDirs)
install(
        TARGETS baudvine-mytex
        PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/baudvine/mytex/
)

# Build the tests (or not)
if (${mytex_TESTS})
    enable_testing()
    add_subdirectory(test)
endif ()
