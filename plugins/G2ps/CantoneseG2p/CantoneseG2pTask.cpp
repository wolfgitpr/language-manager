#include "CantoneseG2pTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Jyutping.h>

#include <InferUtil/ErrorCollector.h>
#include <InferUtil/Parser.h>
#include <InferUtil/Verifier.h>

namespace LangPlugins::CantoneseG2p::V1
{
    class CantoneseG2pTask::Impl {
    public:
        LangCore::NO<LangCore::G2pResultV1> result;
        std::unique_ptr<Pinyin::Jyutping> m_cantonese;
        std::unique_ptr<InferUtil::Verifier> verifier;
        mutable std::shared_mutex mutex;
    };

    CantoneseG2pTask::CantoneseG2pTask(const LangCore::ModuleSpec *spec) :
        Task(spec), _impl(std::make_unique<Impl>()) {}

    CantoneseG2pTask::~CantoneseG2pTask() = default;

    int CantoneseG2pTask::apiLevel() const { return 1; }

    LangCore::Expected<void> CantoneseG2pTask::initialize() {
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
        impl.m_cantonese = std::make_unique<Pinyin::Jyutping>();

        if (!impl.m_cantonese->initialized())
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
    CantoneseG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.m_cantonese->initialized())
                return LangCore::Error(LangCore::Error::SessionError, "CantoneseG2pTask: chinese g2p not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::InvalidArgument, "g2p input is nullptr");

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

            auto pinyinRes = impl.m_cantonese->hanziToPinyin(_input, Pinyin::CanTone::NORMAL, Pinyin::Default, true);

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
} // namespace LangPlugins::CantoneseG2p::V1
