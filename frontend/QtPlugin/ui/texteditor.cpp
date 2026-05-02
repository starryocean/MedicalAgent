#include "TextEditor.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QRegExp>
#include <QTextBlock>
#include <QTextStream>

namespace TranslationAgent {

// ── Line Number Area (from Qt documentation) ─────────────────────────────────

class LineNumberArea : public QWidget {
public:
    LineNumberArea(TextEditor* editor) : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    TextEditor* m_editor;
};

// ── IniSyntaxHighlighter ──────────────────────────────────────────────────────

IniSyntaxHighlighter::IniSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    // Section headers: [SectionName]
    {
        HighlightRule rule;
        rule.pattern = QRegExp("^\\[.*\\]$");
        rule.format.setForeground(QColor("#CBA6F7"));  // Mauve
        rule.format.setFontWeight(QFont::Bold);
        m_rules << rule;
    }
    // Comments: ; or #
    {
        HighlightRule rule;
        rule.pattern = QRegExp("^[;#].*");
        rule.format.setForeground(QColor("#6C7086"));  // Overlay0
        rule.format.setFontItalic(true);
        m_rules << rule;
    }
    // Keys: text before =
    {
        HighlightRule rule;
        rule.pattern = QRegExp("^[^=]+=");
        rule.format.setForeground(QColor("#89B4FA"));  // Blue
        m_rules << rule;
    }
    // Values: text after =
    {
        HighlightRule rule;
        rule.pattern = QRegExp("=(.+)$");
        rule.format.setForeground(QColor("#A6E3A1"));  // Green
        m_rules << rule;
    }
}

void IniSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const auto& rule : m_rules) {
        int index = rule.pattern.indexIn(text);
        while (index >= 0) {
            int length = rule.pattern.matchedLength();
            setFormat(index, length, rule.format);
            index = rule.pattern.indexIn(text, index + length);
        }
    }
}

// ── TextEditor ────────────────────────────────────────────────────────────────

TextEditor::TextEditor(QWidget* parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
{
    setupEditor();

    connect(this, &TextEditor::blockCountChanged,
            this, &TextEditor::updateLineNumberAreaWidth);
    connect(this, &TextEditor::updateRequest,
            this, &TextEditor::updateLineNumberArea);
    connect(this, &TextEditor::selectionChanged,
            this, &TextEditor::onSelectionChanged);

    updateLineNumberAreaWidth(0);
}

void TextEditor::setupEditor() {
    // Catppuccin Mocha dark theme for the editor
    setStyleSheet(
        "QPlainTextEdit {"
        "  background: #1E1E2E;"
        "  color: #CDD6F4;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "  font-size: 13px;"
        "  border: none;"
        "  selection-background-color: #313244;"
        "}"
    );

    QFont font("JetBrains Mono", 13);
    font.setFixedPitch(true);
    setFont(font);

    // Tab width = 4 spaces
    QFontMetrics metrics(font);
    setTabStopWidth(4 * metrics.width(' '));

    m_highlighter = new IniSyntaxHighlighter(document());
}

void TextEditor::loadFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Open File",
            QString("Cannot open file:\n%1").arg(filePath));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    setPlainText(stream.readAll());

    m_filePath = filePath;
    document()->setModified(false);
    emit fileLoaded(filePath);
}

void TextEditor::saveFile() {
    if (m_filePath.isEmpty()) return;

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Save File",
            QString("Cannot save file:\n%1").arg(m_filePath));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << toPlainText();
    document()->setModified(false);
}

QList<IniEntry> TextEditor::selectedEntries() const {
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        // If no selection, use current line
        cursor.select(QTextCursor::LineUnderCursor);
    }

    // Expand selection to full lines
    int selStart = cursor.selectionStart();
    int selEnd   = cursor.selectionEnd();

    QTextCursor startCursor(document());
    startCursor.setPosition(selStart);
    int startLine = startCursor.blockNumber() + 1;

    QTextCursor endCursor(document());
    endCursor.setPosition(selEnd);
    int endLine = endCursor.blockNumber() + 1;

    QList<IniEntry> entries;
    QString currentSection;

    for (QTextBlock block = document()->begin();
         block != document()->end();
         block = block.next())
    {
        int lineNum = block.blockNumber() + 1;
        QString line = block.text().trimmed();

        // Track sections even outside selection
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
        }

        if (lineNum < startLine || lineNum > endLine) continue;
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;
        if (line.startsWith('[')) continue;

        int eqPos = line.indexOf('=');
        if (eqPos > 0) {
            IniEntry entry;
            entry.key        = line.left(eqPos).trimmed();
            entry.value      = line.mid(eqPos + 1).trimmed();
            entry.section    = currentSection;
            entry.lineNumber = lineNum;
            entries << entry;
        }
    }

    return entries;
}

void TextEditor::onSelectionChanged() {
    QList<IniEntry> entries = selectedEntries();
    if (!entries.isEmpty()) {
        emit selectionChanged(entries);
    }
}

void TextEditor::contextMenuEvent(QContextMenuEvent* event) {
    QMenu* menu = createStandardContextMenu();
    menu->addSeparator();

    QList<IniEntry> entries = selectedEntries();
    if (!entries.isEmpty()) {
        QAction* translateAction = menu->addAction(
            QString("Translate %1 selected entr%2")
                .arg(entries.size())
                .arg(entries.size() == 1 ? "y" : "ies")
        );
        connect(translateAction, &QAction::triggered, this, [this]() {
            emit selectionChanged(selectedEntries());
        });
    }

    menu->exec(event->globalPos());
    delete menu;
}

// ── Line Number Gutter ────────────────────────────────────────────────────────

int TextEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 3 + fontMetrics().width('9') * digits + 6;
}

void TextEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void TextEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void TextEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height())
    );
}

void TextEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#181825"));

    QTextBlock block     = firstVisibleBlock();
    int blockNumber      = block.blockNumber();
    int top    = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#45475A"));
            painter.drawText(
                0, top, m_lineNumberArea->width() - 4,
                fontMetrics().height(),
                Qt::AlignRight, number
            );
        }
        block  = block.next();
        top    = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

} // namespace TranslationAgent
