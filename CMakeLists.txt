cmake_minimum_required(VERSION 3.20)
project(ToolDeck VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ---- Dependencies ----
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ---- Sources ----
set(SOURCES
    src/main.cpp
    src/core/tool_manifest.cpp
    src/core/tool_registry.cpp
    src/core/tool_instance.cpp
    src/core/tool_manager.cpp
    src/ui/main_window.cpp
    src/ui/sidebar_widget.cpp
    src/ui/dashboard_widget.cpp
    src/ui/output_panel.cpp
    src/ui/tool_card.cpp
    src/ui/input_dialog.cpp
)

set(HEADERS
    src/core/tool_manifest.h
    src/core/tool_registry.h
    src/core/tool_instance.h
    src/core/tool_manager.h
    src/ui/main_window.h
    src/ui/sidebar_widget.h
    src/ui/dashboard_widget.h
    src/ui/output_panel.h
    src/ui/tool_card.h
    src/ui/input_dialog.h
)

# ---- Executable ----
add_executable(tooldeck ${SOURCES} ${HEADERS})

target_include_directories(tooldeck PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(tooldeck
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# ---- Install ----
install(TARGETS tooldeck RUNTIME DESTINATION bin)
