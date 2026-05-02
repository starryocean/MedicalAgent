/**
 * @file Application.cpp
 * @brief Application-level controller implementation.
 */
#include "Application.h"

#include <QApplication>

#include "network/AgentClient.h"
#include "ui/MainWindow.h"

namespace TranslationAgent {

Application::Application(QObject* parent)
    : QObject(parent)
    , m_client(std::make_unique<AgentClient>())
{}

Application::~Application() = default;

int Application::run(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("TranslationAgent");
    app.setApplicationVersion("1.0.0");

    initializeComponents();

    MainWindow mainWindow;
    mainWindow.show();

    connectSignals();

    return app.exec();
}

void Application::initializeComponents() {
    m_client->checkHealth();
}

void Application::connectSignals() {
    // Wire up application-level signal connections here
}

} // namespace TranslationAgent
