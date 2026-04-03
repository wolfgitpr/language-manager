#ifndef LANGPLUGINS_TEMPLATETAGGER_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_TEMPLATETAGGER_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <shared_mutex>
#include <string>

#include <LangCore/Module/Module.h>

#include "../../TaggerUtil.h"

namespace LangPlugins::TemplateTagger::Internal::V1
{
    /// TemplateTagger 的 Level 1 实现
    /// 使用模板匹配进行语言标记
    class TemplateTaggerTaskImpl final : public LangCore::VersionedTaskImplBase {
    public:
        explicit TemplateTaggerTaskImpl(const LangCore::ModuleSpec *spec);
        ~TemplateTaggerTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        std::unique_ptr<LangPlugins::TemplateTagger::V1::TaggerUtil> m_taggerUtil;
        mutable std::shared_mutex m_mutex;
        std::string m_config;
    };

} // namespace LangPlugins::TemplateTagger::Internal::V1

#endif // LANGPLUGINS_TEMPLATETAGGER_INTERNAL_V1_TASKIMPL_H