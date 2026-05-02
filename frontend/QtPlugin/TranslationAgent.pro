# Translation Agent - Qt project file
# Compatible with Qt 5.6+
# Generated from CMakeLists.txt

QT       += core widgets network

TARGET    = TranslationAgent
TEMPLATE  = app

# C++14 standard (Qt 5.6 compatible)
CONFIG   += c++14

# Enable Qt meta-object, resource, and UI compilers
CONFIG   += automoc
CONFIG   += resources

# ── Sources ───────────────────────────────────────────────────────────────────
SOURCES += \
    main.cpp \
    app/Application.cpp \
    ui/MainWindow.cpp \
    ui/ChatWidget.cpp \
    ui/FileExplorer.cpp \
    ui/TextEditor.cpp \
    ui/DiffViewer.cpp \
    network/AgentClient.cpp \
    models/FileModel.cpp

# ── Headers ───────────────────────────────────────────────────────────────────
HEADERS += \
    app/Application.h \
    ui/MainWindow.h \
    ui/ChatWidget.h \
    ui/FileExplorer.h \
    ui/TextEditor.h \
    ui/DiffViewer.h \
    network/AgentClient.h \
    network/ApiModels.h \
    models/ChatMessage.h \
    models/FileModel.h \
    models/TranslationContext.h

# ── Resources ─────────────────────────────────────────────────────────────────
RESOURCES += \
    resources/resources.qrc

# ── Include path ──────────────────────────────────────────────────────────────
INCLUDEPATH += $$PWD

# ── Platform-specific settings ────────────────────────────────────────────────
win32 {
    # Windows: disable console window in release mode
    CONFIG(release, debug|release) {
        CONFIG += windows
    }
}

macx {
    # macOS: use application bundle
    QMAKE_INFO_PLIST = $$PWD/Info.plist
}

unix:!macx {
    # Linux: no special settings needed
}

# ── Output directories ────────────────────────────────────────────────────────
MOC_DIR     = build/moc
RCC_DIR     = build/rcc
UI_DIR      = build/ui
OBJECTS_DIR = build/obj
DESTDIR     = build/bin
