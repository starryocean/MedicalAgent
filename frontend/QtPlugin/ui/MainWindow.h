/**
 * @file MainWindow.h
 * @brief Application main window - coordinates all UI panels.
 */
#pragma once

#include <QMainWindow>
#include <QSplitter>

#include "models/TranslationContext.h"

namespace TranslationAgent {

class AgentClient;
class ChatWidget;
class DiffViewer;
class FileExplorer;
class TextEditor;

/**
 * @brief Main application window with 3-panel layout:
 * [File Explorer] | [Text Editor] | [Chat + Diff]
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onFileActivated(const QString& filePath);
    void onEditorSelectionChanged(const QList<IniEntry>& entries);
    void onSendRequested(const TranslationContext& context);
    void onTranslationReady(const TranslationResult& result);
    void onTranslationApplied(bool success, const QString& diffText);
    void onDiffAccepted(const TranslationResult& result);
    void onNetworkError(const QString& message);
    void onHealthCheckReady(bool healthy);

    // Menu actions
    void openDirectory();
    void setTargetLanguage();
    void rebuildKnowledgeBase();
    void showAbout();

private:
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void setupConnections();
    void updateStatusBar(const QString& message, bool isError = false);
    TranslationContext buildContext() const;

    // Panels
    FileExplorer* m_fileExplorer;
    TextEditor*   m_editor;
    ChatWidget*   m_chatWidget;
    DiffViewer*   m_diffViewer;
    QSplitter*    m_mainSplitter;
    QSplitter*    m_rightSplitter;

    // Network
    AgentClient*  m_client;

    // State
    QString m_targetLanguage;
    QString m_currentSourceFile;
    QString m_currentTargetFile;
    QString m_sessionId;
};

} // namespace TranslationAgent
