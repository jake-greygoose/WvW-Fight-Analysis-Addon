cmake_minimum_required(VERSION 3.20)
project(WvWFightAnalysis VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate version files before build
if(EXISTS "${CMAKE_SOURCE_DIR}/src/scripts/gen-version.ps1")
    execute_process(
        COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/src/scripts/gen-version.ps1"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src"
    )
endif()

# Source files
set(PROJECT_SOURCES
    # Main files
    src/entry.cpp
    src/gui.cpp

    # GUI sources
    src/src/gui/BarTemplate.cpp
    src/src/gui/WindowRenderer.cpp
    src/src/gui/windows/AggregateWindow.cpp
    src/src/gui/windows/MainWindow.cpp
    src/src/gui/windows/WidgetWindow.cpp

    # Integration
    src/src/integration/MursaatPanelIntegration.cpp

    # Parser
    src/src/parser/boon_strip_skills.cpp
    src/src/parser/boon_strip_tracker.cpp
    src/src/parser/evtc_parser.cpp
    src/src/parser/file_helpers.cpp
    src/src/parser/statistics_helper.cpp

    # Settings
    src/src/settings/Settings.cpp

    # Shared
    src/src/shared/Shared.cpp

    # Utils
    src/src/utils/utils.cpp

    # ImGui
    src/imgui/imgui.cpp
    src/imgui/imgui_demo.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp

    # Miniz
    src/include/thirdparty/miniz.c
    src/include/thirdparty/miniz_tdef.c
    src/include/thirdparty/miniz_tinfl.c
    src/include/thirdparty/miniz_zip.c
)

# Resource files (Windows only)
if(WIN32)
    set(PROJECT_RESOURCES
        src/Resource.rc
        src/autoversion.rc
    )
endif()

# Create the DLL
add_library(${PROJECT_NAME} SHARED
    ${PROJECT_SOURCES}
    ${PROJECT_RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/imgui
    ${CMAKE_SOURCE_DIR}/src/mumble
    ${CMAKE_SOURCE_DIR}/src/nexus
    ${CMAKE_SOURCE_DIR}/src/include/thirdparty
    ${CMAKE_SOURCE_DIR}/src/include/thirdparty/imgui_positioning
)

# Compiler definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    _MBCS
)

# Set output directories for single and multi-config generators
set(_runtime_dir "${CMAKE_BINARY_DIR}/bin")
set(_library_dir "${CMAKE_BINARY_DIR}/lib")

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_runtime_dir}"
    LIBRARY_OUTPUT_DIRECTORY "${_library_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${_library_dir}"
    OUTPUT_NAME "WvWFightAnalysis"
)

foreach(_cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${_cfg}" _cfg_upper)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${_cfg_upper} "${_runtime_dir}/${_cfg}"
        LIBRARY_OUTPUT_DIRECTORY_${_cfg_upper} "${_library_dir}/${_cfg}"
        ARCHIVE_OUTPUT_DIRECTORY_${_cfg_upper} "${_library_dir}/${_cfg}"
    )
endforeach()


# Windows-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        _WINDOWS
    )

    # Link against Windows libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        kernel32
        user32
        gdi32
        winspool
        comdlg32
        advapi32
        shell32
        ole32
        oleaut32
        uuid
        odbc32
        odbccp32
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3
        /MP  # Multi-processor compilation
        $<$<CONFIG:Release>:/O2>  # Optimization for Release
    )

    # Set subsystem to Windows DLL
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /DLL"
    )
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
