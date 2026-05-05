#ifndef TRANSLATIONAGENTPLUGIN_H
#define TRANSLATIONAGENTPLUGIN_H

#include "TranslationAgentGlobal.h"
#include <extensionsystem/iplugin.h>

namespace TranslationAgent {
namespace Internal {

class TranslationAgentPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "TranslationAgent.json")

public:
    TranslationAgentPlugin();
    ~TranslationAgentPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;
};

} // namespace Internal
} // namespace TranslationAgent

#endif // TRANSLATIONAGENTPLUGIN_H
