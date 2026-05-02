/**
 * @file AgentClient.h
 * @brief Async HTTP client for communicating with the Python agent backend.
 * Uses Qt's non-blocking QNetworkAccessManager.
 */
#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>

#include "models/TranslationContext.h"

namespace TranslationAgent {

/**
 * @brief Non-blocking HTTP client for the translation agent API.
 *
 * Design: Fire-and-forget async calls via Qt signals.
 * Caller connects to translationReady/errorOccurred signals.
 */
class AgentClient : public QObject {
    Q_OBJECT

public:
    explicit AgentClient(const QString& baseUrl = "http://localhost:8765",
                         QObject* parent = nullptr);

    void requestTranslation(const TranslationContext& context);
    void applyTranslation(const QString& sourceFile,
                          const QString& targetFile,
                          const QList<TranslationItem>& translations);
    void rebuildKnowledgeBase(bool force = false);
    void checkHealth();
    void clearSession(const QString& sessionId);

    void setBaseUrl(const QString& url) { m_baseUrl = url; }
    QString baseUrl() const             { return m_baseUrl; }

signals:
    void translationReady(const TranslationResult& result);
    void translationApplied(bool success, const QString& diffText);
    void healthCheckReady(bool healthy);
    void kbRebuildStarted();
    void networkError(const QString& message);

private slots:
    void onTranslationReplyFinished();
    void onApplyReplyFinished();
    void onHealthReplyFinished();

private:
    QNetworkReply* postJson(const QString& endpoint, const QByteArray& body);
    QNetworkReply* getJson(const QString& endpoint);
    void           handleNetworkError(QNetworkReply* reply);

    QNetworkAccessManager* m_nam;
    QString                m_baseUrl;
};

} // namespace TranslationAgent
