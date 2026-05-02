/**
 * @file main.cpp
 * @brief Application entry point.
 */
#include <QApplication>
#include <QDir>
#include <QFont>

#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("TranslationAgent");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("YourOrg");

    // Apply system font with fallback
    QFont appFont("Segoe UI", 10);
    appFont.setHintingPreference(QFont::PreferFullHinting);
    app.setFont(appFont);

    // Load global stylesheet
//    QFile styleFile(":/styles/dark_theme.qss");
    QFile styleFile(":/styles/light_theme.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
    }

    TranslationAgent::MainWindow window;
    window.show();

    return app.exec();
}
