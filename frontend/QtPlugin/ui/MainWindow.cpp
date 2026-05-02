#include "MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QUuid>

#include "ChatWidget.h"
#include "DiffViewer.h"
#include "FileExplorer.h"
#include "TextEditor.h"
#include "network/AgentClient.h"

namespace TranslationAgent {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_targetLanguage("English")
    , m_sessionId(QUuid::createUuid().toString().mid(1, 36).left(8))
{
    setupUi();
    setupMenuBar();
    setupStatusBar();
    setupConnections();

    setWindowTitle("Translation Agent");
    setMinimumSize(1200, 700);
    resize(1440, 900);

    // Periodic session cleanup / health check
    auto* healthTimer = new QTimer(this);
    connect(healthTimer, &QTimer::timeout, this, [this]() {
        m_client->checkHealth();
    });
    healthTimer->start(30000); // every 30 seconds

    // Initial health check
    QTimer::singleShot(1000, this, [this]() { m_client->checkHealth(); });
}

void MainWindow::setupUi() {
    // Apply dark theme to entire window
    // Application-wide dark theme is applied via dark_theme.qss
    // This includes QMainWindow, QMenuBar, QMenu, QStatusBar, QSplitter


    // Instantiate panels
    m_fileExplorer = new FileExplorer(this);
    m_editor       = new TextEditor(this);
    m_chatWidget   = new ChatWidget(this);
    m_diffViewer   = new DiffViewer(this);
    m_client       = new AgentClient("http://localhost:8765", this);

    // Right panel: chat on top, diff below
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_rightSplitter->addWidget(m_chatWidget);
    m_rightSplitter->addWidget(m_diffViewer);
    m_rightSplitter->setSizes({500, 400});

    // Main splitter: file tree | editor | right panel
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_fileExplorer);
    m_mainSplitter->addWidget(m_editor);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setSizes({220, 680, 380});

    setCentralWidget(m_mainSplitter);
}

void MainWindow::setupMenuBar() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("Open Directory...", this, &MainWindow::openDirectory,
                        QKeySequence("Ctrl+Shift+O"));
    fileMenu->addSeparator();
    fileMenu->addAction("Save Editor", m_editor, &TextEditor::saveFile,
                        QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", qApp, &QApplication::quit, QKeySequence::Quit);

    // Translation menu
    QMenu* transMenu = menuBar()->addMenu("&Translation");
    transMenu->addAction("Set Target Language...",
                         this, &MainWindow::setTargetLanguage,
                         QKeySequence("Ctrl+L"));
    transMenu->addSeparator();
    transMenu->addAction("Rebuild Knowledge Base",
                         this, &MainWindow::rebuildKnowledgeBase);
    transMenu->addAction("Clear Session",
                         this, [this]() {
                             m_client->clearSession(m_sessionId);
                             m_chatWidget->clearHistory();
                             m_sessionId = QUuid::createUuid()
                                 .toString().mid(1, 36).left(8);
                             updateStatusBar("Session cleared.");
                         });

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("About", this, &MainWindow::showAbout);
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Ready  |  Session: " + m_sessionId);
}

void MainWindow::setupConnections() {
    connect(m_fileExplorer, &FileExplorer::fileActivated,
            this, &MainWindow::onFileActivated);
    connect(m_editor, &TextEditor::selectionChanged,
            this, &MainWindow::onEditorSelectionChanged);
    connect(m_chatWidget, &ChatWidget::sendRequested,
            this, &MainWindow::onSendRequested);
    connect(m_client, &AgentClient::translationReady,
            this, &MainWindow::onTranslationReady);
    connect(m_client, &AgentClient::translationApplied,
            this, &MainWindow::onTranslationApplied);
    connect(m_client, &AgentClient::networkError,
            this, &MainWindow::onNetworkError);
    connect(m_client, &AgentClient::healthCheckReady,
            this, &MainWindow::onHealthCheckReady);
    connect(m_diffViewer, &DiffViewer::accepted,
            this, &MainWindow::onDiffAccepted);
    connect(m_diffViewer, &DiffViewer::rejected, this, [this]() {
        m_diffViewer->clear();
        updateStatusBar("Translation discarded.");
    });
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MainWindow::onFileActivated(const QString& filePath) {
    m_editor->loadFile(filePath);
    m_currentSourceFile = filePath;

    // Auto-detect paired target file (chinese.ini → english.ini)
    QFileInfo fi(filePath);
    QString baseName = fi.baseName().toLower();
    if (baseName.contains("chinese") || baseName.contains("zh")) {
        QString targetName = fi.fileName().replace(
            QRegExp("chinese|zh", Qt::CaseInsensitive), "english"
        );
        m_currentTargetFile = fi.dir().filePath(targetName);
    } else {
        m_currentTargetFile = filePath;
    }

    updateStatusBar(QString("Opened: %1").arg(QFileInfo(filePath).fileName()));
}

void MainWindow::onEditorSelectionChanged(const QList<IniEntry>& entries) {
    TranslationContext ctx = buildContext();
    ctx.selectedEntries = entries;
    m_chatWidget->setTranslationContext(ctx);
    updateStatusBar(
        QString("%1 entries selected -> ready to translate to %2")
            .arg(entries.size()).arg(m_targetLanguage)
    );
}

void MainWindow::onSendRequested(const TranslationContext& context) {
    if (!context.isValid()) {
        updateStatusBar("Please select entries in the editor first.", true);
        return;
    }

    m_chatWidget->setLoading(true);
    updateStatusBar("Translating...");
    m_client->requestTranslation(context);
}

void MainWindow::onTranslationReady(const TranslationResult& result) {
    m_chatWidget->setLoading(false);

    if (!result.success) {
        updateStatusBar("Translation failed: " + result.errorMessage, true);
        m_chatWidget->appendMessage(
            ChatMessage::assistantMessage("Translation failed: " + result.errorMessage)
        );
        return;
    }

    // Show result in chat
    QString summary = QString("Translated %1 entries to %2.\n%3")
        .arg(result.translations.size())
        .arg(m_targetLanguage)
        .arg(result.summary);
    m_chatWidget->appendMessage(ChatMessage::assistantMessage(summary));

    // Show diff viewer for confirmation
    m_diffViewer->showResult(result);
    updateStatusBar(
        QString("Translation ready: %1 entries. Review in diff panel.")
            .arg(result.translations.size())
    );
}

void MainWindow::onDiffAccepted(const TranslationResult& result) {
    m_client->applyTranslation(
        m_currentSourceFile,
        m_currentTargetFile,
        result.translations
    );
    updateStatusBar("Applying translation to " + QFileInfo(m_currentTargetFile).fileName() + "...");
}

void MainWindow::onTranslationApplied(bool success, const QString& diffText) {
    if (success) {
        m_chatWidget->appendMessage(
            ChatMessage::assistantMessage(
                QString("Translation applied to %1 successfully.")
                    .arg(QFileInfo(m_currentTargetFile).fileName())
            )
        );
        updateStatusBar("Translation applied successfully.");

        // Reload editor if viewing the target file
        if (m_editor->currentFilePath() == m_currentTargetFile) {
            m_editor->loadFile(m_currentTargetFile);
        }
    } else {
        updateStatusBar("Failed to apply translation.", true);
    }
    m_diffViewer->clear();
}

void MainWindow::onNetworkError(const QString& message) {
    m_chatWidget->setLoading(false);
    updateStatusBar("Network error: " + message, true);
    m_chatWidget->appendMessage(
        ChatMessage::assistantMessage("Network error: " + message)
    );
}

void MainWindow::onHealthCheckReady(bool healthy) {
    QString status = healthy
        ? QString("Backend online  |  Session: %1  |  Lang: %2")
              .arg(m_sessionId, m_targetLanguage)
        : "Backend offline - check Python server";
    statusBar()->showMessage(status);
}

// ── Menu Actions ──────────────────────────────────────────────────────────────

void MainWindow::openDirectory() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "Open Directory", QDir::homePath()
    );
    if (!dir.isEmpty()) {
        m_fileExplorer->setRootPath(dir);
    }
}

void MainWindow::setTargetLanguage() {
    QStringList languages = {
        "English", "Japanese", "Korean", "German", "French",
        "Spanish", "Russian", "Arabic", "Portuguese", "Italian"
    };
    bool ok;
    QString lang = QInputDialog::getItem(
        this, "Target Language", "Select target language:",
        languages, languages.indexOf(m_targetLanguage), false, &ok
    );
    if (ok && !lang.isEmpty()) {
        m_targetLanguage = lang;
        updateStatusBar("Target language set to: " + lang);
    }
}

void MainWindow::rebuildKnowledgeBase() {
    auto reply = QMessageBox::question(
        this, "Rebuild Knowledge Base",
        "Rebuild the translation knowledge base from Excel files?\n"
        "This may take several minutes.",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        m_client->rebuildKnowledgeBase(true);
        updateStatusBar("Knowledge base rebuild started...");
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "Translation Agent",
        "<h3>Translation Agent v1.0</h3>"
        "<p>AI-powered multi-language translation system</p>"
        "<p>Built with LangChain + Claude + Qt 5</p>"
    );
}

void MainWindow::updateStatusBar(const QString& message, bool isError) {
    if (isError) {
        statusBar()->setProperty("role", "error");
    } else {
        statusBar()->setProperty("role", "normal");
    }
    // Force style refresh after property change
    statusBar()->style()->unpolish(statusBar());
    statusBar()->style()->polish(statusBar());
    statusBar()->showMessage(message, 8000);
}

TranslationContext MainWindow::buildContext() const {
    TranslationContext ctx;
    ctx.targetLanguage  = m_targetLanguage;
    ctx.sessionId       = m_sessionId;
    ctx.sourceFilePath  = m_currentSourceFile;
    ctx.targetFilePath  = m_currentTargetFile;
    ctx.selectedEntries = m_editor->selectedEntries();
    return ctx;
}

} // namespace TranslationAgent
