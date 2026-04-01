#ifndef LANGUAGE_MANAGER_MANAGERLOGGER_H
#define LANGUAGE_MANAGER_MANAGERLOGGER_H

#include <LangCore/Support/Logging.h>

namespace LangCore
{
    inline LogCategory MgrLog("LangCore::Manager");
    inline LogCategory PluginLog("LangCore::Plugin");
    inline LogCategory DependencyLog("LangCore::Dependency");
    inline LogCategory ConfigLog("LangCore::Config");
}
#endif // LANGUAGE_MANAGER_MANAGERLOGGER_H
