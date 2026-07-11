cmake_minimum_required(VERSION 3.20)
project(spindash VERSION 1.0 LANGUAGES CXX)

set(COPYRIGHT "Copyright (c) 2026 Tristan Penman")
set(IDENTIFIER "com.tristanpenman.spindash")

# Force out-of-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent: Easier than Git submodules
include(FetchContent)

# Adds helpers for finding Qt installations and deploying Qt runtime files
include(cmake/QtDeployment.cmake)

# Implements FetchContent steps directly instead of through a sub-build
set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)

# Default to Debug for single-config generators if user didn't specify anything
get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT isMultiConfig AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# -----------------------------------------------------------------------------
# GoogleTest
# -----------------------------------------------------------------------------

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

# -----------------------------------------------------------------------------
# Main executable
# -----------------------------------------------------------------------------

set(PROJECT_SOURCES
    src/main.cc
    src/Chunk.cc
    src/Chunk.h
    src/Command.h
    src/Block.cc
    src/Block.h
    src/BlockDesc.h
    src/Game.h
    src/GameFactory.cc
    src/GameFactory.h
    src/KosinskiReader.cc
    src/KosinskiReader.h
    src/KosinskiWriter.cc
    src/KosinskiWriter.h
    src/Level.h
    src/Logger.cc
    src/Logger.h
    src/Map.cc
    src/Map.h
    src/Palette.cc
    src/Palette.h
    src/Pattern.cc
    src/Pattern.h
    src/PatternDesc.h
    src/Rom.cc
    src/Rom.h
    src/RingGroup.h
    src/commands/PencilCommand.cc
    src/commands/PencilCommand.h
    src/games/Sonic2Rom.cc
    src/games/Sonic2Rom.h
    src/games/Sonic2Disassembly.cc
    src/games/Sonic2Disassembly.h
    src/games/Sonic2Level.cc
    src/games/Sonic2Level.h
    src/games/Sonic2RingLayout.cc
    src/games/Sonic2RingLayout.h
    src/games/Sonic3Rom.cc
    src/games/Sonic3Rom.h
    src/games/Sonic3Level.cc
    src/games/Sonic3Level.h
    src/ui/ChunkInspector.cc
    src/ui/ChunkInspector.h
    src/ui/ChunkSelector.cc
    src/ui/ChunkSelector.h
    src/ui/BlockEditor.cc
    src/ui/BlockEditor.h
    src/ui/BlockInspector.cc
    src/ui/BlockInspector.h
    src/ui/LevelSelect.cc
    src/ui/LevelSelect.h
    src/ui/MapEditor.cc
    src/ui/MapEditor.h
    src/ui/PaletteEditor.cc
    src/ui/PaletteEditor.h
    src/ui/PaletteInspector.cc
    src/ui/PaletteInspector.h
    src/ui/PatternEditor.cc
    src/ui/PatternEditor.h
    src/ui/PatternInspector.cc
    src/ui/PatternInspector.h
    src/ui/ProjectExplorer.cc
    src/ui/ProjectExplorer.h
    src/ui/Rectangle.h
    src/ui/RomInfo.cc
    src/ui/RomInfo.h
    src/ui/ChunkEditor.cc
    src/ui/ChunkEditor.h
    src/ui/Window.cc
    src/ui/Window.h
    src/ui/ZoomSupport.cc
    src/ui/ZoomSupport.h
)

if (APPLE)
  list(APPEND PROJECT_SOURCES
    src/os/macos.h
    src/os/macos.mm
  )
endif()

qt_add_default_windows_prefixes()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets REQUIRED)

if (WIN32)
    add_executable(SpinDash WIN32 ${PROJECT_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/res/SpinDash.rc")
elseif (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "SpinDash.icns")

    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/res/SpinDash.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")

    add_executable(SpinDash MACOSX_BUNDLE ${PROJECT_SOURCES} ${APP_ICON_MACOS})
else()
    add_executable(SpinDash ${PROJECT_SOURCES})
endif()

if(MSVC)
    target_compile_options(SpinDash PRIVATE /W4 /WX)
else()
    target_compile_options(SpinDash PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_target_properties(SpinDash PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(SpinDash PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(WIN32)
    qt_deploy_runtime(SpinDash)
endif()

# -----------------------------------------------------------------------------
# Test suite
# -----------------------------------------------------------------------------

set(TEST_SOURCES
    src/KosinskiReader.h
    src/KosinskiReader.cc
    src/KosinskiWriter.h
    src/KosinskiWriter.cc
    src/games/Sonic2RingLayout.h
    src/games/Sonic2RingLayout.cc
    src/RingGroup.h
    test/data/ehz1.kosinski.h
    test/data/ehz1.raw.h
    test/TestKosinskiReader.cpp
    test/TestKosinskiWriter.cpp
    test/TestSonic2RingLayout.cpp
)

add_executable(SpinDashTest
    ${TEST_SOURCES})

target_link_libraries(SpinDashTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    GTest::gtest_main
)

if(WIN32)
    qt_deploy_runtime(SpinDashTest)
endif()
