#ifndef LANGPLUGINS_LSTMG2P_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_LSTMG2P_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <shared_mutex>
#include <map>
#include <string>

#include <LangCore/Task/SessionTask.h>
#include <LangCore/Task/TaskFactory.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Core/PackageManager.h>

namespace LangPlugins::LstmG2p::Internal::V1
{
    /// LstmG2p 的 Level 1 实现
    /// 使用 LSTM 模型进行文本到音素的转换
    class LstmG2pTaskImpl : public LangCore::VersionedTaskImplBase {
    public:
        explicit LstmG2pTaskImpl(const LangCore::ModuleSpec *spec);
        ~LstmG2pTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        LangCore::NO<LangCore::SessionFactory> m_driver;
        LangCore::NO<LangCore::SessionTask> m_encoderSession;
        LangCore::NO<LangCore::SessionTask> m_decodeSession;
        mutable std::shared_mutex m_mutex;

        std::map<std::string, int> m_charVocab, m_phonemeVocab;
        std::map<int, std::string> m_idxToPhoneme;
        int m_unkIdx = 0;
        int m_padIdx = 1;
        int m_bosIdx = 2;
        int m_eosIdx = 3;
        int m_maxLen = 48;
        std::string m_config;

        // Helper function to load phoneme mapping from JSON file
        static LangCore::Expected<std::map<std::string, int>>
        loadPhonemeMapping(const std::filesystem::path &path, const std::string &fieldName);
    };

} // namespace LangPlugins::LstmG2p::Internal::V1

#endif // LANGPLUGINS_LSTMG2P_INTERNAL_V1_TASKIMPL_H