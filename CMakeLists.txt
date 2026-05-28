cmake_minimum_required(VERSION 3.22)
project(surge-single-effect-lv2 VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(libs/JUCE)

# Dependencies via FetchContent
include(FetchContent)

# SIMDE for ARM/non-x86 SIMD support
FetchContent_Declare(
    simde
    GIT_REPOSITORY https://github.com/simd-everywhere/simde
    GIT_TAG v0.8.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(simde)
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE ${simde_SOURCE_DIR})

# fmt library
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG e424e3f2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(fmt)

add_subdirectory(libs/sst/sst-basic-blocks)
target_link_libraries(sst-basic-blocks INTERFACE simde fmt::fmt)

add_subdirectory(libs/sst/sst-waveshapers)
add_subdirectory(libs/sst/sst-filters)
add_subdirectory(libs/sst/sst-effects)
target_link_libraries(sst-effects INTERFACE sst-waveshapers)

# Build LV2 on all platforms; add VST3 on macOS for Carla/DAW testing
if(APPLE)
    set(PLUGIN_FORMATS VST3 LV2)
elseif(UNIX)
    set(PLUGIN_FORMATS LV2)
endif()

add_subdirectory(src)
