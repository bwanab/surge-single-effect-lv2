# Apply the units:unit LV2 TTL patch to JUCE if not already applied.
# git apply --check returns 0 when the patch can still be applied (not yet applied).
execute_process(
    COMMAND git apply --check ${CMAKE_SOURCE_DIR}/patches/juce-lv2-units.patch
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/JUCE
    RESULT_VARIABLE PATCH_CHECK
    OUTPUT_QUIET ERROR_QUIET
)
if(PATCH_CHECK EQUAL 0)
    execute_process(
        COMMAND git apply ${CMAKE_SOURCE_DIR}/patches/juce-lv2-units.patch
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/JUCE
        RESULT_VARIABLE PATCH_RESULT
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to apply patches/juce-lv2-units.patch")
    endif()
    message(STATUS "Applied JUCE LV2 units:unit patch")
else()
    message(STATUS "JUCE LV2 units:unit patch already applied")
endif()
