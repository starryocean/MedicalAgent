#include "TranslationAgentPlugin.h"
#include "ui/ChatWidget.h"

#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/icore.h>

#include <QAction>
#include <QIcon>

namespace TranslationAgent {
namespace Internal {

/**
 * NavigationWidgetFactory — registers the sidebar widget into the
 * right-hand navigation pane of Qt Creator.
 */
class TranslationNavFactory : public Core::INavigationWidgetFactory
{
public:
    TranslationNavFactory()
    {
        setDisplayName(QStringLiteral("Translation Agent"));
        setPriority(550);
        setId("TranslationAgent.Sidebar");
    }

    Core::NavigationView createWidget() override
    {
        Core::NavigationView view;
        view.widget = new ChatSidebarWidget();
        return view;
    }
};

TranslationAgentPlugin::TranslationAgentPlugin() = default;
TranslationAgentPlugin::~TranslationAgentPlugin() = default;

bool TranslationAgentPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
    addAutoReleasedObject(new TranslationNavFactory);
    return true;
}

void TranslationAgentPlugin::extensionsInitialized() {}

ExtensionSystem::IPlugin::ShutdownFlag TranslationAgentPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace TranslationAgent
