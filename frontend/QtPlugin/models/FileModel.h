/**
 * @file FileModel.h
 * @brief Value object representing a file entry in the file explorer.
 */
#pragma once

#include <QString>

namespace TranslationAgent {

/**
 * @brief Lightweight value object holding metadata about an INI file.
 */
struct FileModel {
    QString filePath;
    QString fileName;
    QString directoryPath;
    bool    isIniFile = false;

    bool isValid() const { return !filePath.isEmpty() && isIniFile; }

    static FileModel fromPath(const QString& path) {
        FileModel model;
        model.filePath      = path;
        model.directoryPath = path.section('/', 0, -2);
        model.fileName      = path.section('/', -1);
        model.isIniFile     = path.endsWith(".ini", Qt::CaseInsensitive);
        return model;
    }
};

} // namespace TranslationAgent
