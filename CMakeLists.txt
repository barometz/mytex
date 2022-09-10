cmake_minimum_required(VERSION 3.15)
project(mytex)

option(mytex_TESTS "Build Mytex tests" ON)
option(mytex_WERROR "Treat warnings as errors" OFF)

add_library(baudvine-mytex INTERFACE)

target_include_directories(baudvine-mytex INTERFACE include)
target_compile_features(baudvine-mytex INTERFACE cxx_std_17)

if(MSVC)
    add_compile_options(
        /W4
    )

    if(${mytex_WERROR})
        add_compile_options(
            /WX
        )
    endif()
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
    )

    if(${mytex_WERROR})
        add_compile_options(
            -Werror
        )
    endif()
endif()

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
if(${mytex_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()
