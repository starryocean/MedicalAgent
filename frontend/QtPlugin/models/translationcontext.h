/**
 * @file TranslationContext.h
 * @brief Aggregates all context needed for a translation request.
 */
#pragma once

#include <QString>
#include <QStringList>

namespace TranslationAgent {

struct IniEntry {
    QString key;
    QString value;
    QString section;
    int     lineNumber = 0;
};

/**
 * @brief Value object capturing a complete translation task context.
 * Passed from the UI layer to the network layer.
 */
struct TranslationContext {
    QList<IniEntry> selectedEntries;  ///< Entries selected in the editor
    QString         targetLanguage;   ///< e.g. "English", "Japanese"
    QString         userMessage;      ///< Additional context from user
    QString         sessionId;        ///< For conversation continuity
    QString         sourceFilePath;   ///< Path to chinese.ini
    QString         targetFilePath;   ///< Path to english.ini

    bool isValid() const {
        return !selectedEntries.isEmpty()
            && !targetLanguage.isEmpty()
            && !sourceFilePath.isEmpty();
    }
};

struct TranslationItem {
    QString key;
    QString source;
    QString target;
    double  confidence  = 0.0;
    QString sourceType; ///< "knowledge_base" | "llm_generated" | "hybrid"
    QString notes;
};

struct TranslationResult {
    bool                    success     = false;
    QString                 summary;
    QStringList             warnings;
    QList<TranslationItem>  translations;
    QString                 diffText;   ///< Unified diff for DiffViewer
    QString                 errorMessage;
};

} // namespace TranslationAgent
