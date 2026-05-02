/**
 * @file ApiModels.h
 * @brief JSON serialization/deserialization for API models.
 */
#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "models/TranslationContext.h"

namespace TranslationAgent {
namespace Api {

/// Serialize TranslationContext to JSON request body
inline QByteArray serializeRequest(const TranslationContext& ctx) {
    QJsonArray entriesArr;
    for (const auto& entry : ctx.selectedEntries) {
        QJsonObject obj;
        obj["key"]         = entry.key;
        obj["value"]       = entry.value;
        obj["section"]     = entry.section;
        obj["line_number"] = entry.lineNumber;
        entriesArr.append(obj);
    }

    QJsonObject root;
    root["entries"]          = entriesArr;
    root["target_language"]  = ctx.targetLanguage;
    root["user_context"]     = ctx.userMessage;
    root["session_id"]       = ctx.sessionId;
    root["source_file_path"] = ctx.sourceFilePath;
    root["target_file_path"] = ctx.targetFilePath;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/// Parse translation response from JSON
inline TranslationResult parseResponse(const QByteArray& data) {
    TranslationResult result;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        result.errorMessage = QString("JSON parse error: %1").arg(parseError.errorString());
        return result;
    }

    QJsonObject root = doc.object();
    result.success   = root["success"].toBool();
    result.summary   = root["summary"].toString();

    for (const auto& w : root["warnings"].toArray()) {
        result.warnings << w.toString();
    }

    for (const auto& t : root["translations"].toArray()) {
        QJsonObject obj = t.toObject();
        TranslationItem item;
        item.key        = obj["key"].toString();
        item.source     = obj["source"].toString();
        item.target     = obj["target"].toString();
        item.confidence = obj["confidence"].toDouble();
        item.sourceType = obj["source_type"].toString();
        item.notes      = obj["notes"].toString();
        result.translations << item;
    }

    if (root.contains("diff_result")) {
        result.diffText = root["diff_result"].toObject()["unified_diff"].toString();
    }

    return result;
}

} // namespace Api
} // namespace TranslationAgent
