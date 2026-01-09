#ifndef LANGMGR_ENGINEFACTORYPLUGIN_H
#define LANGMGR_ENGINEFACTORYPLUGIN_H

#include <LangMgr/Core/Plugin.h>
#include <LangMgr/Task/TaskFactory.h>

namespace LangMgr
{

    class TaskFactoryPlugin : public Plugin {
    public:
        TaskFactoryPlugin() = default;
        ~TaskFactoryPlugin() override = default;

        const char *iid() const override { return "org.openvpi.TaskFactory"; }

        virtual NO<TaskFactory> create() = 0;

        STDCORELIB_DISABLE_COPY(TaskFactoryPlugin)
    };

    class DriverFactoryPlugin : public Plugin {
    public:
        DriverFactoryPlugin() = default;
        ~DriverFactoryPlugin() override = default;

        const char *iid() const override { return "org.openvpi.DriverFactory"; }

        virtual NO<SessionFactory> create() = 0;

        STDCORELIB_DISABLE_COPY(DriverFactoryPlugin)
    };

} // namespace LangMgr

#endif // LANGMGR_ENGINEFACTORYPLUGIN_H
