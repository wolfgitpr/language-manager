#include "TaskImpl.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Support/ConfigAccessor.h>

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Jyutping.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::CantoneseG2p::Internal::V1
{
    CantoneseG2pTaskImpl::CantoneseG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> CantoneseG2pTaskImpl::initialize() {
        std::unique_lock lock(m_mutex);

        m_result.reset();

        auto cfg = LangCore::config(m_spec);

        // Parse verify entries
        auto verifyEntryExp = InferUtil::ParseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }
        auto verifyEntry = verifyEntryExp.take();

        // Required fields
        auto dictPathExp = cfg.getPath("dictPath");
        if (!dictPathExp) {
            return dictPathExp.takeError();
        }
        auto dictPath = dictPathExp.take();

        Pinyin::setDictionaryPath(dictPath);
        m_cantonese = std::make_unique<Pinyin::Jyutping>();

        if (!m_cantonese->initialized())
            return {};

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
    CantoneseG2pTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        {
            std::shared_lock lock(m_mutex);
            if (!m_cantonese->initialized())
                return LangCore::Error(LangCore::Error::RuntimeError, "CantoneseG2pTask: chinese g2p not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        std::vector<LangCore::G2pRes> res;
        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        // Parse verify entries
        auto cfg = LangCore::config(m_spec);
        auto verifyEntryExp = InferUtil::ParseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }
        InferUtil::VerifierHolder verifierHolder(verifyEntryExp.take());

        const auto verifyRes = verifierHolder.verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, verifyError] : verifyRes) {
            LangCore::G2pErrorType wordErrorType = LangCore::NoError;
            if (!verifyError.empty()) {
                wordErrorType = LangCore::InvalidLyric;
            }
            res.emplace_back(LangCore::G2pRes{
                std::string(lyric), std::string(m_spec->name().text()), std::string(), std::vector<std::string>(), std::string(mode), wordErrorType});
        }

        const auto groupLyric = groupLyrics(res);

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();

        for (const auto &g2pResGroup : groupLyric) {
            const auto mode = g2pResGroup.front().mode;
            std::vector<std::string> _input;
            for (const auto &g2pRes : g2pResGroup)
                _input.push_back(g2pRes.lyric);

            auto pinyinRes = m_cantonese->hanziToPinyin(_input, Pinyin::CanTone::NORMAL, Pinyin::Default, true);

            for (auto &[hanzi, pinyin, candidates, conversionError] : pinyinRes) {
                LangCore::G2pErrorType wordErrorType = LangCore::NoError;
                if (conversionError) {
                    wordErrorType = LangCore::PinyinConversionFailed;
                }
                LangCore::G2pRes newRes;
                newRes.lyric = std::string(hanzi);
                newRes.g2pId = std::string(m_spec->id());
                newRes.pronunciation = std::string(mode == "convert" ? pinyin : hanzi);
                newRes.candidates = std::vector<std::string>();
                newRes.mode = std::string(mode);
                newRes.errorType = wordErrorType;
                g2pResult->g2pResult.emplace_back(newRes);
            }
        }

        m_result = g2pResult;
        return g2pResult;
    }

    std::string CantoneseG2pTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> CantoneseG2pTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }

} // namespace LangPlugins::CantoneseG2p::Internal::V1