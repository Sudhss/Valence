cmake_minimum_required(VERSION 3.16)

project(Valence VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(APP_ICON_RESOURCE_WINDOWS app.rc)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp

    # Core
    src/core/text_buffer.h
    src/core/text_buffer.cpp
    src/core/selection.h
    src/core/undo_manager.h
    src/core/undo_manager.cpp

    # Editor
    src/editor/editor_widget.h
    src/editor/editor_widget.cpp
    src/editor/cpp_highlighter.h
    src/editor/cpp_highlighter.cpp

    # UI
    src/ui/main_window.h
    src/ui/main_window.cpp
    src/ui/tab_widget.h
    src/ui/tab_widget.cpp
    src/ui/file_explorer.h
    src/ui/file_explorer.cpp
    src/ui/terminal_widget.h
    src/ui/terminal_widget.cpp
    src/ui/status_bar.h
    src/ui/status_bar.cpp

    # Theme
    src/theme/theme.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Valence
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    qt_add_executable(Valence
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${APP_ICON_RESOURCE_WINDOWS}
    
)   
endif()

target_link_libraries(Valence PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(Valence PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(Valence PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "Valence"
)

include(GNUInstallDirs)
install(TARGETS Valence
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Valence)
endif()
