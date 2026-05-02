#include "AgentClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

#include "ApiModels.h"

namespace TranslationAgent {

AgentClient::AgentClient(const QString& baseUrl, QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_baseUrl(baseUrl)
{}

void AgentClient::requestTranslation(const TranslationContext& context) {
    QByteArray body = Api::serializeRequest(context);
    QNetworkReply* reply = postJson("/api/v1/translate", body);

    connect(reply, &QNetworkReply::finished, this, &AgentClient::onTranslationReplyFinished);
    reply->setProperty("requestBody", body);
}

void AgentClient::applyTranslation(const QString& sourceFile,
                                    const QString& targetFile,
                                    const QList<TranslationItem>& translations) {
    QJsonArray transArr;
    for (const auto& t : translations) {
        QJsonObject obj;
        obj["key"]         = t.key;
        obj["source"]      = t.source;
        obj["target"]      = t.target;
        obj["confidence"]  = t.confidence;
        obj["source_type"] = t.sourceType;
        obj["notes"]       = t.notes;
        transArr.append(obj);
    }

    QJsonObject root;
    root["source_file_path"] = sourceFile;
    root["target_file_path"] = targetFile;
    root["translations"]     = transArr;

    QNetworkReply* reply = postJson(
        "/api/v1/apply-translation",
        QJsonDocument(root).toJson(QJsonDocument::Compact)
    );
    connect(reply, &QNetworkReply::finished, this, &AgentClient::onApplyReplyFinished);
}

void AgentClient::checkHealth() {
    QNetworkReply* reply = getJson("/api/v1/health");
    connect(reply, &QNetworkReply::finished, this, &AgentClient::onHealthReplyFinished);
}

void AgentClient::rebuildKnowledgeBase(bool force) {
    QJsonObject root;
    root["force"] = force;
    postJson("/api/v1/kb/rebuild", QJsonDocument(root).toJson(QJsonDocument::Compact));
    emit kbRebuildStarted();
}

void AgentClient::clearSession(const QString& sessionId) {
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/session/" + sessionId));
    m_nam->deleteResource(request);
}

// ── Private Slots ─────────────────────────────────────────────────────────────

void AgentClient::onTranslationReplyFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }

    TranslationResult result = Api::parseResponse(reply->readAll());
    emit translationReady(result);
}

void AgentClient::onApplyReplyFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    QString diff    = obj["diff"].toString();
    emit translationApplied(obj["success"].toBool(), diff);
}

void AgentClient::onHealthReplyFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    bool healthy = (reply->error() == QNetworkReply::NoError);
    if (healthy) {
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        healthy = obj["status"].toString() == "healthy";
    }
    emit healthCheckReady(healthy);
}

// ── Private Helpers ───────────────────────────────────────────────────────────

QNetworkReply* AgentClient::postJson(const QString& endpoint, const QByteArray& body) {
    QNetworkRequest request(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, body.size());
    return m_nam->post(request, body);
}

QNetworkReply* AgentClient::getJson(const QString& endpoint) {
    QNetworkRequest request(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return m_nam->get(request);
}

void AgentClient::handleNetworkError(QNetworkReply* reply) {
    QString msg = QString("Network error [%1]: %2")
        .arg(reply->error())
        .arg(reply->errorString());
    emit networkError(msg);
}

} // namespace TranslationAgent
