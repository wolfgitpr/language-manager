#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/Task.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Task/SplitterTask.h>
#include <LangCore/Task/VersionedTaskImplBase.h>

namespace LangPlugins::RegexSplitter::Internal
{
    class TaskImplBase;
}

namespace LangPlugins::RegexSplitter::Internal::V1
{
    class RegexSplitterTaskImpl : public LangCore::VersionedTaskImplBase {
    public:
        explicit RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;
        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        std::string m_pattern;
        std::string m_config;
    };
} // namespace LangPlugins::RegexSplitter::Internal::V1

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_V1_TASKIMPL_H