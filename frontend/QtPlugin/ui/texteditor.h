/**
 * @file TextEditor.h
 * @brief INI file text editor with selection-to-context functionality.
 *
 * Supports:
 * - Syntax highlighting for INI format
 * - Multi-line selection tracking
 * - Right-click context menu for translation
 * - Line number gutter (CodeEditor pattern)
 */
#pragma once

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

#include "models/TranslationContext.h"

namespace TranslationAgent {

/**
 * @brief INI syntax highlighter.
 * Highlights sections, keys, values, and comments distinctly.
 */
class IniSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit IniSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightRule {
        QRegExp          pattern;
        QTextCharFormat  format;
    };
    QList<HighlightRule> m_rules;
};

/**
 * @brief Code editor with line number gutter (Qt documentation pattern).
 */
class TextEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit TextEditor(QWidget* parent = nullptr);

    void loadFile(const QString& filePath);
    void saveFile();
    bool hasUnsavedChanges() const { return document()->isModified(); }
    QString currentFilePath() const { return m_filePath; }

    /// Parse selected lines into IniEntry list for translation context
    QList<IniEntry> selectedEntries() const;

    /// Line number gutter width
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent* event);

signals:
    void selectionChanged(const QList<IniEntry>& entries);
    void fileLoaded(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void onSelectionChanged();

private:
    void setupEditor();
    void parseSelectedLines(const QString& text,
                             int startLine,
                             QList<IniEntry>& out) const;

    QWidget*              m_lineNumberArea;
    IniSyntaxHighlighter* m_highlighter;
    QString               m_filePath;
};

} // namespace TranslationAgent
