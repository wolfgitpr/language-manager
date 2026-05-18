#ifndef LANGPLUGINS_COMMON_PINYING2PTASKIMPLBASE_H
#define LANGPLUGINS_COMMON_PINYING2PTASKIMPLBASE_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
#include <filesystem>

#include <LangCore/Task/G2pTask.h>
#include <LangCore/Core/PackageManager.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <InferUtil/Verifier.h>
#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/PinyinRes.h>

namespace LangPlugins::Common
{

class PinyinG2pTaskImplBase : public LangCore::VersionedTaskImplBase {
public:
    struct Config {
        std::string dictPathKey;
        std::string languageName;
    };

    explicit PinyinG2pTaskImplBase(const LangCore::ModuleSpec *spec, Config config);
    ~PinyinG2pTaskImplBase() override = default;

    LangCore::Expected<void> initialize() override final;

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    start(const LangCore::NO<LangCore::TaskInput> &input) override final;

    std::string getConfig() const override final;

protected:
    virtual LangCore::Expected<void> onInitializeEngine() = 0;
    virtual bool isEngineInitialized() const = 0;
    virtual std::vector<Pinyin::PinyinRes> doHanziToPinyin(
        const std::vector<std::string> &input) = 0;

    const LangCore::ModuleSpec *m_spec;
    std::filesystem::path m_dictPath;

private:
    static std::vector<std::vector<LangCore::G2pRes>>
    groupLyrics(const std::vector<LangCore::G2pRes> &input);

    Config m_langConfig;
    LangCore::NO<LangCore::G2pResultV1> m_result;
    std::unique_ptr<LangPlugins::InferUtil::Verifier> m_verifier;
    mutable std::shared_mutex m_mutex;
    mutable std::string m_config;
};

} // namespace LangPlugins::Common

#endif // LANGPLUGINS_COMMON_PINYING2PTASKIMPLBASE_H