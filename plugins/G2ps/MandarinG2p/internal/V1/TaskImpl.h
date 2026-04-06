#ifndef LANGPLUGINS_MANDARING2P_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_MANDARING2P_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
#include <filesystem>

#include <cpp-pinyin/Pinyin.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Core/PackageManager.h>
#include <InferUtil/Verifier.h>

namespace LangPlugins::MandarinG2p::Internal::V1
{
    /// MandarinG2p 的 Level 1 实现
    /// 使用 cpp-pinyin 库进行中文转拼音
    class MandarinG2pTaskImpl final : public LangCore::VersionedTaskImplBase {
    public:
        explicit MandarinG2pTaskImpl(const LangCore::ModuleSpec *spec);
        ~MandarinG2pTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

    private:
        const LangCore::ModuleSpec *m_spec;
        LangCore::NO<LangCore::G2pResultV1> m_result;
        std::unique_ptr<Pinyin::Pinyin> m_mandarin;
        mutable std::shared_mutex m_mutex;
        std::string m_config;
        
        // 配置私有成员变量
        std::filesystem::path m_dictPath;
        std::vector<LangPlugins::InferUtil::VerifyEntry> m_verifyEntries;
    };

} // namespace LangPlugins::MandarinG2p::Internal::V1

#endif // LANGPLUGINS_MANDARING2P_INTERNAL_V1_TASKIMPL_H