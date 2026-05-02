/**
 * @file ChatMessage.h
 * @brief Immutable chat message value object.
 * Follows Qt naming conventions and value semantics.
 */
#pragma once

#include <QDateTime>
#include <QString>

namespace TranslationAgent {

/**
 * @brief Represents a single message in the chat dialog.
 * Designed as a value type - cheap to copy, immutable after construction.
 */
class ChatMessage {
public:
    enum class Role { User, Assistant, System };

    ChatMessage() = default;

    ChatMessage(Role role, const QString& content)
        : m_role(role)
        , m_content(content)
        , m_timestamp(QDateTime::currentDateTime())
    {}

    Role          role()      const { return m_role; }
    QString       content()   const { return m_content; }
    QDateTime     timestamp() const { return m_timestamp; }

    bool isFromUser()      const { return m_role == Role::User; }
    bool isFromAssistant() const { return m_role == Role::Assistant; }

    static ChatMessage userMessage(const QString& content) {
        return ChatMessage(Role::User, content);
    }

    static ChatMessage assistantMessage(const QString& content) {
        return ChatMessage(Role::Assistant, content);
    }

    static ChatMessage systemMessage(const QString& content) {
        return ChatMessage(Role::System, content);
    }

private:
    Role      m_role      = Role::System;
    QString   m_content;
    QDateTime m_timestamp;
};

} // namespace TranslationAgent
