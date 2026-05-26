cmake_minimum_required(VERSION 3.22)
project(surge-single-effect-lv2 VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Apply units:unit patch to JUCE's LV2 TTL writer before JUCE is configured
include(cmake/ApplyJuceUnitsPatch.cmake)

add_subdirectory(libs/JUCE)
add_subdirectory(libs/sst/sst-basic-blocks)
add_subdirectory(libs/sst/sst-filters)
add_subdirectory(libs/sst/sst-effects)

# Build LV2 on all platforms; add VST3 on macOS for Carla/DAW testing
if(APPLE)
    set(PLUGIN_FORMATS VST3 LV2)
elseif(UNIX)
    set(PLUGIN_FORMATS LV2)
endif()

add_subdirectory(src)
