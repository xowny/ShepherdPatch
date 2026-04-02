cmake_minimum_required(VERSION 3.24)

project(ShepherdPatch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(SHH_CORE_SOURCES
    src/bink_playback.cpp
    src/config.cpp
    src/crash_dump.cpp
    src/d3d_proc_redirect.cpp
    src/direct_input_policy.cpp
    src/engine_fov.cpp
    src/engine_vars_overrides.cpp
    src/execution_policy.cpp
    src/frame_rate_override.cpp
    src/gameplay_delta.cpp
    src/intro_skip.cpp
    src/legacy_runtime_policy.cpp
    src/lost_device_policy.cpp
    src/memory_utils.cpp
    src/present_params.cpp
    src/precise_sleep.cpp
    src/projection_fix.cpp
    src/qpc_timer.cpp
    src/raw_mouse.cpp
    src/render_hook_targets.cpp
    src/runtime.cpp
    src/scheduler_timing.cpp
    src/scheduler_targets.cpp
    src/thread_diagnostics.cpp
    src/timing_diagnostics.cpp
    src/window_mode.cpp
    src/viewport_fix.cpp
)

add_library(ShepherdPatch SHARED
    src/dllmain.cpp
    ${SHH_CORE_SOURCES}
)

target_include_directories(ShepherdPatch PRIVATE src)
target_compile_definitions(ShepherdPatch PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ShepherdPatch PRIVATE dinput8 dxguid dbghelp)
set_target_properties(ShepherdPatch PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "ShepherdPatch"
)

add_library(version SHARED
    src/version.def
    src/version_proxy.cpp
)

target_include_directories(version PRIVATE src)
target_compile_definitions(version PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
set_target_properties(version PROPERTIES
    PREFIX ""
    OUTPUT_NAME "version"
)

add_executable(ShepherdPatchTests
    tests/test_bink_playback.cpp
    tests/test_config.cpp
    tests/test_crash_dump.cpp
    tests/test_d3d_proc_redirect.cpp
    tests/test_direct_input_policy.cpp
    tests/test_engine_fov.cpp
    tests/test_engine_vars_overrides.cpp
    tests/test_execution_policy.cpp
    tests/test_frame_rate_override.cpp
    tests/test_gameplay_delta.cpp
    tests/test_intro_skip.cpp
    tests/test_legacy_runtime_policy.cpp
    tests/test_lost_device_policy.cpp
    tests/test_memory_utils.cpp
    tests/test_precise_sleep.cpp
    tests/test_scheduler_timing.cpp
    tests/test_scheduler_targets.cpp
    tests/test_thread_diagnostics.cpp
    tests/test_viewport_fix.cpp
    tests/test_main.cpp
    tests/test_present_params.cpp
    tests/test_projection_fix.cpp
    tests/test_qpc_timer.cpp
    tests/test_raw_mouse.cpp
    tests/test_render_hook_targets.cpp
    tests/test_timing_diagnostics.cpp
    tests/test_window_mode.cpp
    src/config.cpp
    src/bink_playback.cpp
    src/crash_dump.cpp
    src/d3d_proc_redirect.cpp
    src/direct_input_policy.cpp
    src/engine_fov.cpp
    src/engine_vars_overrides.cpp
    src/execution_policy.cpp
    src/frame_rate_override.cpp
    src/gameplay_delta.cpp
    src/intro_skip.cpp
    src/legacy_runtime_policy.cpp
    src/lost_device_policy.cpp
    src/memory_utils.cpp
    src/precise_sleep.cpp
    src/present_params.cpp
    src/projection_fix.cpp
    src/qpc_timer.cpp
    src/raw_mouse.cpp
    src/render_hook_targets.cpp
    src/scheduler_timing.cpp
    src/scheduler_targets.cpp
    src/thread_diagnostics.cpp
    src/timing_diagnostics.cpp
    src/window_mode.cpp
    src/viewport_fix.cpp
)

target_include_directories(ShepherdPatchTests PRIVATE src)
target_compile_definitions(ShepherdPatchTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

enable_testing()
add_test(NAME ShepherdPatchTests COMMAND ShepherdPatchTests)
