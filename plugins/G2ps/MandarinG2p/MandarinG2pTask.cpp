#include "MandarinG2pTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Pinyin.h>

#include <InferUtil/ErrorCollector.h>
#include <InferUtil/Parser.h>
#include <InferUtil/Verifier.h>

namespace LangPlugins::MandarinG2p::V1
{
    class MandarinG2pTask::Impl {
    public:
        LangCore::NO<LangCore::G2pResultV1> result;
        std::unique_ptr<Pinyin::Pinyin> m_mandarin;
        std::unique_ptr<InferUtil::Verifier> verifier;
        mutable std::shared_mutex mutex;
    };

    MandarinG2pTask::MandarinG2pTask(const LangCore::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    MandarinG2pTask::~MandarinG2pTask() = default;

    int MandarinG2pTask::apiLevel() const { return 1; }

    LangCore::Expected<void> MandarinG2pTask::initialize() {
        __stdc_impl_t;

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec(), &ec);

        std::filesystem::path dictPath;
        std::vector<InferUtil::VerifyEntry> verifyEntry;

        parser.parse_verify_required(verifyEntry, "verify");
        parser.parse_path_required(dictPath, "dictPath");

        auto expVerifier = InferUtil::Verifier::Create(verifyEntry);
        if (!expVerifier)
            return expVerifier.takeError();
        impl.verifier = expVerifier.take();

        Pinyin::setDictionaryPath(dictPath);
        impl.m_mandarin = std::make_unique<Pinyin::Pinyin>();

        if (!impl.m_mandarin->initialized())
            return {};

        // return success
        return {};
    }

    static std::vector<std::vector<LangCore::G2pRes>> groupLyrics(const std::vector<LangCore::G2pRes> &input) {
        std::vector<std::vector<LangCore::G2pRes>> groups;
        std::string lastMode;

        for (const auto &item : input) {
            if (groups.empty() || item.mode != lastMode) {
                groups.emplace_back();
                lastMode = item.mode;
            }
            groups.back().push_back(item);
        }

        return groups;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    MandarinG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.m_mandarin->initialized())
                return LangCore::Error(LangCore::Error::RuntimeError, "MandarinG2pTask: chinese g2p not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        std::vector<LangCore::G2pRes> res;
        const auto g2pInput = input.as<LangCore::G2pInputV1>();
        const auto verifyRes = impl.verifier->verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangCore::G2pRes{
                lyric, spec()->name().text(), "", {}, mode, error, error ? LangCore::InvalidLyric : LangCore::NoError});

        const auto groupLyric = groupLyrics(res);

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();

        for (const auto &g2pResGroup : groupLyric) {
            const auto mode = g2pResGroup.front().mode;
            std::vector<std::string> _input;
            for (const auto &g2pRes : g2pResGroup)
                _input.push_back(g2pRes.lyric);

            auto pinyinRes =
                impl.m_mandarin->hanziToPinyin(_input, Pinyin::ManTone::NORMAL, Pinyin::Default, true, false, false);

            for (auto &[hanzi, pinyin, candidates, error] : pinyinRes) {
                g2pResult->g2pResult.emplace_back(hanzi, spec()->id(), mode == "convert" ? pinyin : hanzi, candidates,
                                                  mode, mode == "convert" && error,
                                                  mode == "convert" && error ? LangCore::G2pDepInternalError
                                                                             : LangCore::NoError);
            }
        }

        impl.result = g2pResult;
        return g2pResult;
    }

    LangCore::Expected<void> MandarinG2pTask::updateConfig(const std::string &config) {
        // 简单实现：将配置存储到 Task 基类中
        // 具体的配置解析和更新逻辑可以在需要时由插件自行实现
        return setConfig(config);
    }
} // namespace LangPlugins::MandarinG2p::V1
