#ifndef LANGCORE_ENGINEFACTORYPLUGIN_H
#define LANGCORE_ENGINEFACTORYPLUGIN_H

#include <LangCore/Core/Plugin.h>
#include <LangCore/Task/TaskFactory.h>

namespace LangCore
{
    class TaskPlugin : public Plugin {
    public:
        TaskPlugin() = default;
        ~TaskPlugin() override = default;

        const char *iid() const override { return "org.openvpi.Task"; }

        virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;

        STDCORELIB_DISABLE_COPY(TaskPlugin)
    };

    class DriverPlugin : public Plugin {
    public:
        DriverPlugin() = default;
        ~DriverPlugin() override = default;

        const char *iid() const override { return "org.openvpi.Driver"; }

        virtual Expected<NO<SessionFactory>> create() = 0;

        STDCORELIB_DISABLE_COPY(DriverPlugin)
    };

} // namespace LangCore

#endif // LANGCORE_ENGINEFACTORYPLUGIN_H
