cmake_minimum_required(VERSION 3.20)
project(UnifiedEngineeringWorkbench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static MSVC runtime for clean standalone deployment
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/W4 /permissive- /utf-8)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
)

# -----------------------------------------------------------------------------
# Core Static Library: wb_core
# -----------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/fem1d.cpp
    src/core/stl_parser.cpp
    src/core/csv_parser.cpp
    src/core/batch.cpp
    src/core/backprop.cpp
    src/core/devices.cpp
    src/core/sysinfo.cpp
    src/core/export.cpp
)

add_library(wb_core STATIC ${CORE_SOURCES})
target_include_directories(wb_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# -----------------------------------------------------------------------------
# Third-party UI Library: wb_ui_deps
# -----------------------------------------------------------------------------
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_dx11.cpp
    third_party/implot/implot.cpp
    third_party/implot/implot_items.cpp
)

add_library(wb_ui_deps STATIC ${IMGUI_SOURCES})
target_include_directories(wb_ui_deps PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
)

# -----------------------------------------------------------------------------
# Automated Test Suite: wb_tests
# -----------------------------------------------------------------------------
add_executable(wb_tests tests/test_main.cpp)
target_link_libraries(wb_tests PRIVATE wb_core)

# -----------------------------------------------------------------------------
# Command-Line Tools
# -----------------------------------------------------------------------------
# wb_fem: Standalone FEM runner & result exporter
add_executable(wb_fem src/app/cli_fem.cpp)
target_link_libraries(wb_fem PRIVATE wb_core)

# wb_bench: Standalone multicore benchmark runner
add_executable(wb_bench src/app/cli_bench.cpp)
target_link_libraries(wb_bench PRIVATE wb_core)

# wb_backprop: Standalone backpropagation trainer & exporter
add_executable(wb_backprop src/app/cli_backprop.cpp)
target_link_libraries(wb_backprop PRIVATE wb_core)

# -----------------------------------------------------------------------------
# Main Standalone Desktop Workbench: Launch.exe
# -----------------------------------------------------------------------------
add_executable(Launch WIN32 src/app/main.cpp)
target_link_libraries(Launch PRIVATE
    wb_core
    wb_ui_deps
    d3d11
    d3dcompiler
    dxgi
)

# -----------------------------------------------------------------------------
# Installation Target for Packaging
# -----------------------------------------------------------------------------
install(TARGETS Launch DESTINATION bin)
install(DIRECTORY data/ DESTINATION data)
install(DIRECTORY docs/ DESTINATION docs)

