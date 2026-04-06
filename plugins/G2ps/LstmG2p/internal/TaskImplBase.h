#ifndef LANGPLUGINS_LSTMG2P_INTERNAL_TASKIMPLBASE_H
#define LANGPLUGINS_LSTMG2P_INTERNAL_TASKIMPLBASE_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <shared_mutex>
#include <map>
#include <string>
#include <filesystem>

#include <LangCore/Task/SessionTask.h>
#include <LangCore/Task/TaskFactory.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Core/PackageManager.h>

namespace LangPlugins::LstmG2p::Internal
{
    /// LstmG2p 的基类实现
    /// 包含 V1 和 V2 的共同代码
    class LstmG2pTaskImplBase : public LangCore::VersionedTaskImplBase {
    public:
        explicit LstmG2pTaskImplBase(const LangCore::ModuleSpec *spec);
        ~LstmG2pTaskImplBase() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override = 0;

        std::string getConfig() const override;

    protected:
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
        
        // 配置私有成员变量
        std::filesystem::path m_encoderPath;
        std::filesystem::path m_decoderPath;
        std::filesystem::path m_charVocabPath;
        std::filesystem::path m_phonemeVocabPath;

        // Helper function to load phoneme mapping from JSON file
        static LangCore::Expected<std::map<std::string, int>>
        loadPhonemeMapping(const std::filesystem::path &path, const std::string &fieldName);
    };

} // namespace LangPlugins::LstmG2p::Internal

#endif // LANGPLUGINS_LSTMG2P_INTERNAL_TASKIMPLBASE_H