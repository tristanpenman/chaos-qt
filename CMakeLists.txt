cmake_minimum_required(VERSION 3.20)
project(chaos-qt VERSION 1.0 LANGUAGES CXX)

set(COPYRIGHT "Copyright (c) 2026 Tristan Penman")
set(IDENTIFIER "com.tristanpenman.chaos-qt")

# Force out-of-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent: Easier than Git submodules
include(FetchContent)

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
    src/main.cpp
    src/Block.cpp
    src/Block.h
    src/Command.h
    src/Chunk.cpp
    src/Chunk.h
    src/ChunkDesc.h
    src/Game.h
    src/GameFactory.cpp
    src/GameFactory.h
    src/KosinskiReader.cpp
    src/KosinskiReader.h
    src/KosinskiWriter.cpp
    src/KosinskiWriter.h
    src/Level.h
    src/Logger.cpp
    src/Logger.h
    src/Map.cpp
    src/Map.h
    src/Palette.cpp
    src/Palette.h
    src/Pattern.cpp
    src/Pattern.h
    src/PatternDesc.h
    src/Rom.cpp
    src/Rom.h
    src/commands/PencilCommand.cpp
    src/commands/PencilCommand.h
    src/games/Sonic2.cpp
    src/games/Sonic2.h
    src/games/Sonic2Level.cpp
    src/games/Sonic2Level.h
    src/games/Sonic3.cpp
    src/games/Sonic3.h
    src/games/Sonic3Level.cpp
    src/games/Sonic3Level.h
    src/ui/BlockInspector.cpp
    src/ui/BlockInspector.h
    src/ui/BlockSelector.cpp
    src/ui/BlockSelector.h
    src/ui/ChunkInspector.cpp
    src/ui/ChunkInspector.h
    src/ui/LevelSelect.cpp
    src/ui/LevelSelect.h
    src/ui/MapEditor.cpp
    src/ui/MapEditor.h
    src/ui/PaletteInspector.cpp
    src/ui/PaletteInspector.h
    src/ui/PatternInspector.cpp
    src/ui/PatternInspector.h
    src/ui/Rectangle.h
    src/ui/RomInfo.cpp
    src/ui/RomInfo.h
    src/ui/Window.cpp
    src/ui/Window.h
    src/ui/ZoomSupport.cpp
    src/ui/ZoomSupport.h
)

if (APPLE)
  list(APPEND PROJECT_SOURCES
    src/os/macos.h
    src/os/macos.mm
  )
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets REQUIRED)

if (WIN32)
    add_executable(Chaos ${PROJECT_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/res/Chaos.rc")
elseif (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "Chaos.icns")

    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/res/Chaos.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")

    add_executable(Chaos MACOSX_BUNDLE ${PROJECT_SOURCES} ${APP_ICON_MACOS})
else()
    add_executable(Chaos ${PROJECT_SOURCES})
endif()

if(MSVC)
    target_compile_options(Chaos PRIVATE /W4 /WX)
else()
    target_compile_options(Chaos PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_target_properties(Chaos PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(Chaos PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# -----------------------------------------------------------------------------
# Test suite
# -----------------------------------------------------------------------------

set(TEST_SOURCES
    src/KosinskiReader.h
    src/KosinskiReader.cpp
    src/KosinskiWriter.h
    src/KosinskiWriter.cpp
    test/data/ehz1.kosinski.h
    test/data/ehz1.raw.h
    test/TestKosinskiReader.cpp
    test/TestKosinskiWriter.cpp
)

add_executable(ChaosTest
    ${TEST_SOURCES})

target_link_libraries(ChaosTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    GTest::gtest_main
)
