#include "ChatWidget.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMovie>
#include <QPushButton>
#include <QScrollBar>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QFileInfo>

namespace TranslationAgent {

// ── MessageBubble ─────────────────────────────────────────────────────────────

MessageBubble::MessageBubble(const ChatMessage& message, QWidget* parent)
    : QWidget(parent)
{
    setupUi(message);
}

void MessageBubble::setupUi(const ChatMessage& message) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);

    m_contentLabel   = new QLabel(this);
    m_timestampLabel = new QLabel(this);

    m_contentLabel->setWordWrap(true);
    m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_contentLabel->setText(message.content());

    m_timestampLabel->setObjectName("timestampLabel");
    m_timestampLabel->setText(message.timestamp().toString("HH:mm"));
    m_timestampLabel->setAlignment(Qt::AlignBottom);

    bool isUser = message.isFromUser();

    // Cursor-style: user messages right-aligned, assistant left-aligned
    if (isUser) {
        layout->addStretch();
        layout->addWidget(m_contentLabel);
        layout->addWidget(m_timestampLabel);
        m_contentLabel->setProperty("role", "user");
    } else {
        layout->addWidget(m_timestampLabel);
        layout->addWidget(m_contentLabel);
        layout->addStretch();
        m_contentLabel->setProperty("role", "assistant");
    }
}

// ── ChatWidget ────────────────────────────────────────────────────────────────

ChatWidget::ChatWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setupConnections();
}

void ChatWidget::setupUi() {
    setMinimumWidth(320);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Messages area ──────────────────────────────────────────────
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // QScrollArea and its container styled via QSS object selectors

    m_messagesContainer = new QWidget();
    m_messagesContainer->setObjectName("messagesContainer");
    m_messagesLayout    = new QVBoxLayout(m_messagesContainer);
    m_messagesLayout->setSpacing(4);
    m_messagesLayout->setContentsMargins(8, 8, 8, 8);
    m_messagesLayout->addStretch(); // Push messages to bottom initially

    m_scrollArea->setWidget(m_messagesContainer);

    // ── Loading indicator ──────────────────────────────────────────
    m_loadingLabel = new QLabel("Translating...", this);
    m_loadingLabel->setObjectName("loadingLabel");
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->hide();

    // ── Input area ─────────────────────────────────────────────────
    auto* inputFrame = new QWidget(this);
    inputFrame->setObjectName("inputFrame");
    auto* inputLayout = new QHBoxLayout(inputFrame);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(6);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setObjectName("inputEdit");
    m_inputEdit->setPlaceholderText(
        "Ask to translate... (Enter to send, Shift+Enter for new line)"
    );
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->setMinimumHeight(38);

    m_sendButton = new QPushButton(QString::fromUtf8("\xe2\x86\x91"), this);
     m_sendButton->setObjectName("sendButton");
    m_sendButton->setFixedSize(34, 34);
    m_sendButton->setToolTip("Send translation request (Enter)");

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendButton, 0, Qt::AlignBottom);

    rootLayout->addWidget(m_scrollArea, 1);
    rootLayout->addWidget(m_loadingLabel);
    rootLayout->addWidget(inputFrame);
}

void ChatWidget::setupConnections() {
    connect(m_sendButton, &QPushButton::clicked,
            this, &ChatWidget::onSendClicked);

    // Enter to send, Shift+Enter for newline - Cursor-style
    m_inputEdit->installEventFilter(this);
}

bool ChatWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return
            && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            onSendClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ChatWidget::onSendClicked() {
    const QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    if (!m_currentContext.isValid() && text.isEmpty()) return;

    m_currentContext.userMessage = text;
    m_inputEdit->clear();

    // Append user message to chat
    ChatMessage userMsg = ChatMessage::userMessage(
        QString("[→ %1]\n%2")
            .arg(m_currentContext.targetLanguage)
            .arg(text.left(100))
    );
    appendMessage(userMsg);

    emit sendRequested(m_currentContext);
}

void ChatWidget::onInputReturnPressed() {

}

void ChatWidget::appendMessage(const ChatMessage& message) {
    m_history << message;

    // Remove the trailing stretch before adding message
    QLayoutItem* stretch = m_messagesLayout->takeAt(m_messagesLayout->count() - 1);

    auto* bubble = new MessageBubble(message, m_messagesContainer);
    m_messagesLayout->addWidget(bubble);
    m_messagesLayout->addStretch();

    QTimer::singleShot(50, this, &ChatWidget::scrollToBottom);
    delete stretch;
}

void ChatWidget::scrollToBottom() {
    m_scrollArea->verticalScrollBar()->setValue(
        m_scrollArea->verticalScrollBar()->maximum()
    );
}

void ChatWidget::setTranslationContext(const TranslationContext& ctx) {
    m_currentContext = ctx;

    if (!ctx.selectedEntries.isEmpty()) {
        QString preview = QString("%1 entries selected from %2 → %3")
            .arg(ctx.selectedEntries.size())
            .arg(QFileInfo(ctx.sourceFilePath).fileName())
            .arg(ctx.targetLanguage);

        auto* hint = new QLabel(preview, m_messagesContainer);
        hint->setObjectName("contextHint");
        // Insert before the last stretch
        m_messagesLayout->insertWidget(m_messagesLayout->count() - 1, hint);
        QTimer::singleShot(50, this, &ChatWidget::scrollToBottom);
    }
}

void ChatWidget::setLoading(bool loading) {
    m_loadingLabel->setVisible(loading);
    m_sendButton->setEnabled(!loading);
    m_inputEdit->setEnabled(!loading);
}

void ChatWidget::clearHistory() {
    m_history.clear();
    // Remove all widgets from messages layout except the trailing stretch
    while (m_messagesLayout->count() > 1) {
        QLayoutItem* item = m_messagesLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

} // namespace TranslationAgent
