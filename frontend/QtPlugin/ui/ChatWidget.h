/**
 * @file ChatWidget.h
 * @brief Cursor-style chat dialog widget.
 *
 * Renders a scrollable message list with distinct user/assistant bubbles,
 * an input area with send button, and loading state indicators.
 */
#pragma once

#include <QScrollArea>
#include <QTextEdit>
#include <QWidget>

#include "models/ChatMessage.h"
#include "models/TranslationContext.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace TranslationAgent {

/**
 * @brief Individual message bubble widget.
 * Renders one ChatMessage with appropriate styling per role.
 */
class MessageBubble : public QWidget {
    Q_OBJECT
public:
    explicit MessageBubble(const ChatMessage& message, QWidget* parent = nullptr);

private:
    void setupUi(const ChatMessage& message);
    QLabel* m_contentLabel;
    QLabel* m_timestampLabel;
};

/**
 * @brief Main chat interface widget mimicking Cursor's chat panel.
 *
 * Features:
 * - Scrollable message history with auto-scroll to bottom
 * - Multi-line input with Shift+Enter for newlines
 * - Visual loading indicator during API calls
 * - Translation context injection from editor selection
 */
class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget* parent = nullptr);

    void appendMessage(const ChatMessage& message);
    void setTranslationContext(const TranslationContext& ctx);
    void setLoading(bool loading);
    void clearHistory();

signals:
    void sendRequested(const TranslationContext& context);

private slots:
    void onSendClicked();
    void onInputReturnPressed();
    void scrollToBottom();

private:
    void setupUi();
    void setupConnections();
    bool eventFilter(QObject* obj, QEvent* event) override;
    QString buildContextualMessage() const;

    // Layout
    QScrollArea*  m_scrollArea;
    QWidget*      m_messagesContainer;
    QVBoxLayout*  m_messagesLayout;
    QTextEdit*    m_inputEdit;
    QPushButton*  m_sendButton;
    QLabel*       m_loadingLabel;

    // State
    TranslationContext m_currentContext;
    QList<ChatMessage> m_history;
};

} // namespace TranslationAgent
