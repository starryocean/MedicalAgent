/**
 * @file DiffViewer.h
 * @brief Side-by-side diff viewer for comparing INI translations.
 */
#pragma once

#include "models/TranslationContext.h"
#include <QWidget>

class QTextEdit;
class QPushButton;
class QLabel;

namespace TranslationAgent {

struct TranslationResult;

/**
 * @brief Displays unified diff output with syntax highlighting.
 * Shows original vs. translated INI content for user confirmation.
 */
class DiffViewer : public QWidget {
    Q_OBJECT

public:
    explicit DiffViewer(QWidget* parent = nullptr);

    void showResult(const TranslationResult& result);
    void clear();

signals:
    void accepted(const TranslationResult& result);
    void rejected();

private slots:
    void onAcceptClicked();
    void onRejectClicked();

private:
    void setupUi();
    void applyDiffHighlighting(QTextEdit* editor, const QString& diffText);
    QString renderTranslationTable(const TranslationResult& result) const;

    QTextEdit*   m_diffEdit;
    QTextEdit*   m_summaryEdit;
    QPushButton* m_acceptButton;
    QPushButton* m_rejectButton;
    QLabel*      m_statsLabel;

    TranslationResult m_pendingResult;
};

} // namespace TranslationAgent
