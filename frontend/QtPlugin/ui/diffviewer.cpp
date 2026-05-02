#include "DiffViewer.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

#include "models/TranslationContext.h"

namespace TranslationAgent {

DiffViewer::DiffViewer(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void DiffViewer::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ─────────────────────────────────────────────────────
    auto* header = new QLabel("Translation Review", this);
    header->setStyleSheet(
        "background: #181825; color: #CDD6F4; font-weight: bold;"
        "font-size: 13px; padding: 8px 12px;"
        "border-bottom: 1px solid #313244;"
    );

    // ── Stats bar ──────────────────────────────────────────────────
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet(
        "background: #1E1E2E; color: #A6ADC8; font-size: 12px;"
        "padding: 4px 12px; border-bottom: 1px solid #313244;"
    );

    // ── Translation table ──────────────────────────────────────────
    m_summaryEdit = new QTextEdit(this);
    m_summaryEdit->setReadOnly(true);
    m_summaryEdit->setMinimumHeight(200);
    m_summaryEdit->setStyleSheet(
        "QTextEdit { background: #1E1E2E; color: #CDD6F4; border: none;"
        "font-family: 'JetBrains Mono', monospace; font-size: 12px; }"
    );

    // ── Diff view ──────────────────────────────────────────────────
    m_diffEdit = new QTextEdit(this);
    m_diffEdit->setReadOnly(true);
    m_diffEdit->setMaximumHeight(150);
    m_diffEdit->setStyleSheet(
        "QTextEdit { background: #11111B; color: #CDD6F4; border: none;"
        "font-family: 'JetBrains Mono', monospace; font-size: 11px; }"
    );

    // ── Action buttons ─────────────────────────────────────────────
    auto* buttonBar = new QWidget(this);
    buttonBar->setStyleSheet("background: #181825; border-top: 1px solid #313244;");
    auto* btnLayout = new QHBoxLayout(buttonBar);
    btnLayout->setContentsMargins(12, 8, 12, 8);

    m_rejectButton = new QPushButton("Discard", this);
    m_rejectButton->setStyleSheet(
        "QPushButton { background: #45475A; color: #CDD6F4; border-radius: 6px;"
        "padding: 7px 18px; font-size: 13px; border: none; }"
        "QPushButton:hover { background: #585B70; }"
    );

    m_acceptButton = new QPushButton("Apply to english.ini", this);
    m_acceptButton->setStyleSheet(
        "QPushButton { background: #40A02B; color: white; border-radius: 6px;"
        "padding: 7px 18px; font-size: 13px; border: none; font-weight: bold; }"
        "QPushButton:hover { background: #4CC32E; }"
    );

    btnLayout->addWidget(m_rejectButton);
    btnLayout->addStretch();
    btnLayout->addWidget(m_acceptButton);

    root->addWidget(header);
    root->addWidget(m_statsLabel);
    root->addWidget(m_summaryEdit, 3);
    root->addWidget(m_diffEdit, 1);
    root->addWidget(buttonBar);

    connect(m_acceptButton, &QPushButton::clicked, this, &DiffViewer::onAcceptClicked);
    connect(m_rejectButton, &QPushButton::clicked, this, &DiffViewer::onRejectClicked);
}

void DiffViewer::showResult(const TranslationResult& result) {
    m_pendingResult = result;

    // Stats bar
    int kbCount  = 0;
    int llmCount = 0;
    for (const auto& t : result.translations) {
        if (t.sourceType == "knowledge_base") ++kbCount;
        else ++llmCount;
    }

    m_statsLabel->setText(
        QString("  %1 translations  |  %2 from knowledge base  |  %3 LLM-generated  |  Warnings: %4")
            .arg(result.translations.size())
            .arg(kbCount)
            .arg(llmCount)
            .arg(result.warnings.size())
    );

    // Translation table
    m_summaryEdit->setHtml(renderTranslationTable(result));

    // Diff view
    if (!result.diffText.isEmpty()) {
        applyDiffHighlighting(m_diffEdit, result.diffText);
    } else {
        m_diffEdit->setPlainText("(diff will be available after applying)");
    }

    m_acceptButton->setEnabled(result.success);
}

QString DiffViewer::renderTranslationTable(const TranslationResult& result) const {
    QString html = R"(
<style>
  body { font-family: 'JetBrains Mono', monospace; font-size: 12px; color: #CDD6F4; }
  table { width: 100%; border-collapse: collapse; }
  th { background: #313244; padding: 6px 10px; text-align: left;
       border-bottom: 2px solid #45475A; }
  td { padding: 5px 10px; border-bottom: 1px solid #1E1E2E; vertical-align: top; }
  .key   { color: #89B4FA; width: 20%; }
  .src   { color: #FAB387; width: 35%; }
  .tgt   { color: #A6E3A1; width: 35%; }
  .badge { padding: 2px 6px; border-radius: 10px; font-size: 10px; }
  .kb    { background: #313244; color: #CBA6F7; }
  .llm   { background: #1E1E2E; color: #89B4FA; }
  .conf  { color: #A6ADC8; width: 10%; text-align: center; }
  .warn  { background: #FAB387; color: #1E1E2E; padding: 4px 8px;
           border-radius: 4px; margin: 4px 0; }
</style>
)";

    if (!result.warnings.isEmpty()) {
        html += "<div style='padding: 8px;'>";
        for (const auto& w : result.warnings) {
            html += QString("<div class='warn'>%1</div>").arg(w.toHtmlEscaped());
        }
        html += "</div>";
    }

    html += "<table><tr>"
            "<th class='key'>Key</th>"
            "<th class='src'>Source (Chinese)</th>"
            "<th class='tgt'>Translation</th>"
            "<th class='conf'>Conf.</th>"
            "</tr>";

    for (const auto& t : result.translations) {
        QString badgeClass = (t.sourceType == "knowledge_base") ? "kb" : "llm";
        QString badgeText  = (t.sourceType == "knowledge_base") ? "KB" : "AI";

        html += QString(
            "<tr>"
            "<td class='key'><code>%1</code></td>"
            "<td class='src'>%2</td>"
            "<td class='tgt'>%3 <span class='badge %4'>%5</span></td>"
            "<td class='conf'>%6%</td>"
            "</tr>"
        )
        .arg(t.key.toHtmlEscaped())
        .arg(t.source.toHtmlEscaped())
        .arg(t.target.toHtmlEscaped())
        .arg(badgeClass)
        .arg(badgeText)
        .arg(static_cast<int>(t.confidence * 100));
    }

    if (!result.summary.isEmpty()) {
        html += QString(
            "<tr><td colspan='4' style='color: #A6ADC8; font-style: italic;"
            "padding: 8px;'>%1</td></tr>"
        ).arg(result.summary.toHtmlEscaped());
    }

    html += "</table>";
    return html;
}

void DiffViewer::applyDiffHighlighting(QTextEdit* editor, const QString& diffText) {
    editor->clear();
    QTextCursor cursor = editor->textCursor();

    for (const QString& line : diffText.split('\n')) {
        QTextCharFormat fmt;

        if (line.startsWith('+') && !line.startsWith("+++")) {
            fmt.setForeground(QColor("#A6E3A1"));  // Green: added
            fmt.setBackground(QColor("#1A2E1A"));
        } else if (line.startsWith('-') && !line.startsWith("---")) {
            fmt.setForeground(QColor("#F38BA8"));  // Red: removed
            fmt.setBackground(QColor("#2E1A1A"));
        } else if (line.startsWith("@@")) {
            fmt.setForeground(QColor("#89B4FA"));  // Blue: hunk header
        } else if (line.startsWith("+++") || line.startsWith("---")) {
            fmt.setForeground(QColor("#CBA6F7"));  // Purple: file header
            fmt.setFontWeight(QFont::Bold);
        } else {
            fmt.setForeground(QColor("#A6ADC8"));  // Gray: context
        }

        cursor.insertText(line + '\n', fmt);
    }
}

void DiffViewer::clear() {
    m_summaryEdit->clear();
    m_diffEdit->clear();
    m_statsLabel->clear();
    m_pendingResult = TranslationResult{};
}

void DiffViewer::onAcceptClicked() {
    emit accepted(m_pendingResult);
}

void DiffViewer::onRejectClicked() {
    clear();
    emit rejected();
}

} // namespace TranslationAgent
