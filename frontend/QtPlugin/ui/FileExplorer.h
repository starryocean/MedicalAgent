/**
 * @file FileExplorer.h
 * @brief File system tree browser with INI file focus.
 */
#pragma once

#include <QDockWidget>
#include <QFileSystemModel>
#include <QTreeView>

namespace TranslationAgent {

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    QString currentFilePath() const;

signals:
    void fileActivated(const QString& filePath);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onFilterChanged(const QString& text);

private:
    void setupUi();

    QTreeView*        m_treeView;
    QFileSystemModel* m_model;
    QLineEdit*        m_filterEdit;
};

} // namespace TranslationAgent
