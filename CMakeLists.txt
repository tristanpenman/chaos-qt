cmake_minimum_required(VERSION 3.20)
project(Chaos VERSION 1.0 LANGUAGES CXX)

set(COPYRIGHT "Copyright (c) 2026 Tristan Penman")
set(IDENTIFIER "com.tristanpenman.chaos")

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# -----------------------------------------------------------------------------
#
# Main executable
#
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
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/res/Chaos.rc")
elseif (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "Chaos.icns")

    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/res/Chaos.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")

    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES} ${APP_ICON_MACOS})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# -----------------------------------------------------------------------------
#
# Test executable
#
# -----------------------------------------------------------------------------

set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT for GoogleTest." FORCE)
add_subdirectory(thirdparty/googletest)

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

add_executable(${PROJECT_NAME}Test ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}Test PRIVATE
    gtest
    gtest_main
)

target_compile_features(${PROJECT_NAME}Test PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME}Test SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
