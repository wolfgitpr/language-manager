#ifndef LANGPLUGINSDSDICT_DSDICTTASK_H
#define LANGPLUGINSDSDICT_DSDICTTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/DictTask.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::DsDict
{
    class DsDictTask : public LangCore::Task {
    public:
        explicit DsDictTask(const LangCore::ModuleSpec *spec);
        ~DsDictTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

    private:
        LangCore::VersionedTaskManager _manager;
    };

} // namespace LangPlugins::DsDict

#endif // LANGPLUGINSDSDICT_DSDICTTASK_H