cmake_minimum_required(VERSION 3.5)
project(TranslationAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.6 REQUIRED COMPONENTS
    Core Widgets Network
)

set(SOURCES
    main.cpp
    app/Application.cpp
    ui/MainWindow.cpp
    ui/ChatWidget.cpp
    ui/FileExplorer.cpp
    ui/TextEditor.cpp
    ui/DiffViewer.cpp
    network/AgentClient.cpp
    models/FileModel.cpp
)

set(HEADERS
    app/Application.h
    ui/MainWindow.h
    ui/ChatWidget.h
    ui/FileExplorer.h
    ui/TextEditor.h
    ui/DiffViewer.h
    network/AgentClient.h
    network/ApiModels.h
    models/ChatMessage.h
    models/FileModel.h
    models/TranslationContext.h
)

set(RESOURCES resources/resources.qrc)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)