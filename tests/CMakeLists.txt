# Code Modification - Build System: Exercise the baseline and the actual pinned SDK ABI in both configurations.
# Scope: test executables only. Performance: no shipping cost. Toggle: WISEENGINE_BUILD_TESTS.
add_executable(BaselineTests BaselineTests.cpp)
target_link_libraries(BaselineTests PRIVATE Core)
target_compile_definitions(BaselineTests PRIVATE WISEENGINE_EXPECTED_VERSION="${PROJECT_VERSION}")
wiseengine_target(BaselineTests)
add_test(NAME Core.Baseline COMMAND BaselineTests)
add_test(NAME App.Version COMMAND App --version)
set_tests_properties(App.Version PROPERTIES PASS_REGULAR_EXPRESSION "WiseEngine ${PROJECT_VERSION}")
add_test(NAME App.BuildInfo COMMAND App --build-info)
set_tests_properties(App.BuildInfo PROPERTIES PASS_REGULAR_EXPRESSION "configuration=${CMAKE_BUILD_TYPE}")
add_test(NAME App.InvalidArgument COMMAND App --unknown)
set_tests_properties(App.InvalidArgument PROPERTIES WILL_FAIL TRUE)
add_test(NAME Build.ModuleBoundaries.RejectReverseLink COMMAND ${CMAKE_COMMAND}
    -DWISEENGINE_SOURCE=${PROJECT_SOURCE_DIR} -DWISEENGINE_TEST_BINARY=${CMAKE_CURRENT_BINARY_DIR}/boundary-fixture
    -P "${CMAKE_CURRENT_SOURCE_DIR}/ExpectBoundaryFailure.cmake")
add_executable(SDKSmoke SDKSmoke.cpp)
wiseengine_target(SDKSmoke)
target_link_libraries(SDKSmoke PRIVATE
    ThirdParty::fmt ThirdParty::spdlog ThirdParty::yaml-cpp
    ThirdParty::SDL ThirdParty::glm ThirdParty::Boost ThirdParty::EnTT
    ThirdParty::enkiTS ThirdParty::physfs ThirdParty::asio
    ThirdParty::Luau ThirdParty::LuaBridge3 ThirdParty::Jolt ThirdParty::Tracy
    ThirdParty::json ThirdParty::zstd ThirdParty::xxHash ThirdParty::meshoptimizer
    ThirdParty::DirectXTex ThirdParty::miniaudio ThirdParty::stb ThirdParty::Diligent)
wiseengine_stage_dxc(SDKSmoke)
add_test(NAME SDK.Integration COMMAND SDKSmoke)
add_test(NAME SDK.Shader.DXIL COMMAND ThirdParty::dxc -T cs_6_0 -E main
    -Fo "${CMAKE_CURRENT_BINARY_DIR}/sdk-smoke.dxil" "${CMAKE_CURRENT_SOURCE_DIR}/SDKSmoke.hlsl")
if(WISEENGINE_ENABLE_VULKAN)
    add_test(NAME SDK.Shader.SPIRV COMMAND ThirdParty::dxc -spirv -T cs_6_0 -E main
        -Fo "${CMAKE_CURRENT_BINARY_DIR}/sdk-smoke.spv" "${CMAKE_CURRENT_SOURCE_DIR}/SDKSmoke.hlsl")
endif()
set_tests_properties(Core.Baseline App.Version App.BuildInfo App.InvalidArgument SDK.Integration SDK.Shader.DXIL PROPERTIES TIMEOUT 60)
# End of Code Modification - Build System

add_test(NAME Build.BackupRecovery COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/BackupTests.py")

# Code Modification - Tests: Cover startup rollback and input through the real SDL event queue.
# Scope: test target only. Performance: no shipping cost. Toggle: WISEENGINE_BUILD_TESTS.
add_executable(ApplicationShellTests ApplicationShellTests.cpp ../src/App/ApplicationShell.cpp)
target_link_libraries(ApplicationShellTests PRIVATE Core Input ThirdParty::spdlog)
wiseengine_target(ApplicationShellTests)
add_test(NAME App.Shell COMMAND ApplicationShellTests)
set_tests_properties(App.Shell PROPERTIES TIMEOUT 15)
# End of Code Modification - Tests

# Code Modification - Tests: Validate the platform report, coordinate limits, and separate clocks.
# Scope: test targets. Performance: no shipping cost. Toggle: WISEENGINE_BUILD_TESTS.
add_executable(FoundationTests FoundationTests.cpp ../src/App/PlatformInfo.cpp ../src/App/ElapsedClock.cpp)
target_link_libraries(FoundationTests PRIVATE Math Core Renderer ThirdParty::Boost ThirdParty::SDL)
wiseengine_target(FoundationTests)
add_test(NAME Foundation.Contracts COMMAND FoundationTests)
add_test(NAME App.PlatformInfo COMMAND App --platform-info)
set_tests_properties(Foundation.Contracts App.PlatformInfo PROPERTIES
    TIMEOUT 15 ENVIRONMENT "SDL_VIDEO_DRIVER=wiseengine-invalid-driver")
set_tests_properties(App.PlatformInfo PROPERTIES PASS_REGULAR_EXPRESSION "performance_counter_hz=[1-9][0-9]*")
# End of Code Modification - Tests

# Code Modification - Tests: Exercise scheduler dependencies and shutdown with real workers.
# Scope: test executables and headless CLI. Performance: test-only. Toggle: WISEENGINE_BUILD_TESTS.
add_executable(JobSystemTests JobSystemTests.cpp)
target_link_libraries(JobSystemTests PRIVATE Jobs)
wiseengine_target(JobSystemTests)
add_test(NAME Jobs.Contracts COMMAND JobSystemTests)
add_test(NAME App.JobSmoke COMMAND App --job-smoke)
set_tests_properties(Jobs.Contracts App.JobSmoke PROPERTIES
    TIMEOUT 20 ENVIRONMENT "SDL_VIDEO_DRIVER=wiseengine-invalid-driver")
# End of Code Modification - Tests

# Code Modification - Tests: Verify scratch ownership and generation-safe handles.
# Scope: Core contract test. Performance: test-only. Toggle: WISEENGINE_BUILD_TESTS.
add_executable(MemoryContractsTests MemoryContractsTests.cpp)
target_link_libraries(MemoryContractsTests PRIVATE Core)
wiseengine_target(MemoryContractsTests)
add_test(NAME Core.MemoryContracts COMMAND MemoryContractsTests)
set_tests_properties(Core.MemoryContracts PROPERTIES TIMEOUT 15)
# End of Code Modification - Tests
