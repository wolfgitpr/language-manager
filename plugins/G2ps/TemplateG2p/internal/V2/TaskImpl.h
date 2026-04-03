#ifndef LANGPLUGINS_TEMPLATEG2P_INTERNAL_V2_TASKIMPL_H
#define LANGPLUGINS_TEMPLATEG2P_INTERNAL_V2_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <vector>
#include <shared_mutex>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Support/PhonemeDict.h>
#include <LangCore/Core/PackageManager.h>

namespace LangPlugins::TemplateG2p::Internal::V2
{
    /// TemplateG2p 的 Level 2 实现
    /// 支持字典查找和 ONNX 推理（支持批量处理）
    class TemplateG2pTaskImpl final : public LangCore::VersionedTaskImplBase {
    public:
        explicit TemplateG2pTaskImpl(const LangCore::ModuleSpec *spec);
        ~TemplateG2pTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        LangCore::NO<LangCore::Task> m_g2pInference;
        bool m_enableOnnxG2p{};
        bool m_enableDict{};
        LangCore::PhonemeDict m_phonemeDict;
        mutable std::shared_mutex m_mutex;
        std::string m_config;

        std::vector<std::string> lookup(const std::string &key) const;
    };

} // namespace LangPlugins::TemplateG2p::Internal::V2

#endif // LANGPLUGINS_TEMPLATEG2P_INTERNAL_V2_TASKIMPL_H