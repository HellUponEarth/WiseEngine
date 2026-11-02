# This negative fixture proves that CMake really rejects an otherwise linkable static-module cycle.
if(NOT DEFINED WISEENGINE_SOURCE OR NOT DEFINED WISEENGINE_TEST_BINARY)
    message(FATAL_ERROR "Source and test binary paths are required.")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${WISEENGINE_SOURCE}/tests/boundaries"
    -B "${WISEENGINE_TEST_BINARY}" "-DWISEENGINE_SOURCE=${WISEENGINE_SOURCE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT "${output}${error}" MATCHES "Engine module boundary violation: Core -> Input")
    message(FATAL_ERROR "Expected reverse dependency rejection was not observed: ${output}${error}")
endif()
