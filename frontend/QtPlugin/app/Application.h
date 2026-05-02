/**
 * @file Application.h
 * @brief Application-level controller managing lifecycle and shared state.
 */
#pragma once

#include <QObject>
#include <memory>

namespace TranslationAgent {

class AgentClient;
class MainWindow;

/**
 * @brief Application controller that wires up the main window and network client.
 * Single Responsibility: Only application initialization and lifecycle.
 */
class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    int run(int argc, char* argv[]);

signals:
    void aboutToQuit();

private:
    void initializeComponents();
    void connectSignals();

    std::unique_ptr<AgentClient> m_client;
};

} // namespace TranslationAgent
