#include "FileExplorer.h"

#include <QDir>
#include <QHeaderView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

namespace TranslationAgent {

FileExplorer::FileExplorer(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void FileExplorer::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Filter input
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Filter files...");
    m_filterEdit->setStyleSheet(
        "QLineEdit { background: #313244; color: #CDD6F4; border: none;"
        "padding: 6px 10px; font-size: 12px; }"
        "QLineEdit:focus { border-bottom: 2px solid #89B4FA; }"
    );

    // File system model - show only INI files and directories
    m_model = new QFileSystemModel(this);
    m_model->setNameFilters({"*.ini", "*.INI"});
    m_model->setNameFilterDisables(false);
    m_model->setRootPath(QDir::homePath());

    // Proxy for text filtering
    auto* proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_model);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
//    proxy->setRecursiveFilteringEnabled(true);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(proxy);
    m_treeView->setRootIndex(proxy->mapFromSource(
        m_model->index(QDir::homePath())
    ));
    m_treeView->header()->hideSection(1); // Size
    m_treeView->header()->hideSection(2); // Type
    m_treeView->header()->hideSection(3); // Date
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_treeView->setStyleSheet(
        "QTreeView { background: #181825; color: #CDD6F4; border: none;"
        "font-size: 12px; }"
        "QTreeView::item:hover    { background: #313244; }"
        "QTreeView::item:selected { background: #2563EB; color: white; }"
        "QTreeView::branch:has-children:!has-siblings:closed,"
        "QTreeView::branch:closed:has-children:has-siblings {"
        "  border-image: none; }"
    );

    layout->addWidget(m_filterEdit);
    layout->addWidget(m_treeView, 1);

    connect(m_treeView, &QTreeView::doubleClicked,
            this, &FileExplorer::onItemDoubleClicked);
    connect(m_filterEdit, &QLineEdit::textChanged,
            this, &FileExplorer::onFilterChanged);
}

void FileExplorer::setRootPath(const QString& path) {
    m_model->setRootPath(path);
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(m_treeView->model());
    if (proxy) {
        m_treeView->setRootIndex(proxy->mapFromSource(m_model->index(path)));
    }
}

void FileExplorer::onItemDoubleClicked(const QModelIndex& index) {
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(m_treeView->model());
    QModelIndex sourceIndex = proxy ? proxy->mapToSource(index) : index;

    QString path = m_model->filePath(sourceIndex);
    if (!m_model->isDir(sourceIndex) && path.endsWith(".ini", Qt::CaseInsensitive)) {
        emit fileActivated(path);
    }
}

void FileExplorer::onFilterChanged(const QString& text) {
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(m_treeView->model());
    if (proxy) {
        proxy->setFilterWildcard(text.isEmpty() ? "*" : ("*" + text + "*"));
    }
}

} // namespace TranslationAgent
