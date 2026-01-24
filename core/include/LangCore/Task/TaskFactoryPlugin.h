#ifndef LANGCORE_ENGINEFACTORYPLUGIN_H
#define LANGCORE_ENGINEFACTORYPLUGIN_H

#include <LangCore/Core/Plugin.h>
#include <LangCore/Task/TaskFactory.h>

namespace LangCore
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

} // namespace LangCore

#endif // LANGCORE_ENGINEFACTORYPLUGIN_H
