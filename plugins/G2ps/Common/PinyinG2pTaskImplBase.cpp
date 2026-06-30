#include "PinyinG2pTaskImplBase.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Logging.h>

#include <cpp-pinyin/G2pglobal.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::Common
{
    using namespace LangPlugins::InferUtil;

    PinyinG2pTaskImplBase::PinyinG2pTaskImplBase(const LangCore::ModuleSpec *spec, Config config)
        : m_spec(spec), m_langConfig(std::move(config)) {}

    LangCore::Expected<void> PinyinG2pTaskImplBase::initialize() {
        std::unique_lock lock(m_mutex);

        m_result.reset();

        auto cfg = LangCore::config(m_spec);

        auto verifyEntryExp = ParseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }

        auto verifierExp = Verifier::Create(verifyEntryExp.take());
        if (!verifierExp) {
            return verifierExp.takeError();
        }
        m_verifier = verifierExp.take();

        auto dictPathExp = cfg.getResolvedPath(m_langConfig.dictPathKey);
        if (!dictPathExp) {
            return dictPathExp.takeError();
        }
        m_dictPath = dictPathExp.take();

        Pinyin::setDictionaryPath(m_dictPath);

        auto initResult = onInitializeEngine();
        if (!initResult) {
            return initResult;
        }

        m_config = getConfig();

        if (!isEngineInitialized())
            return LangCore::Error(LangCore::Error::InitializationError,
                                   stdc::formatN("%1: cpp-pinyin library failed to initialize", m_langConfig.languageName));

        return {};
    }

    std::vector<std::vector<LangCore::G2pRes>>
    PinyinG2pTaskImplBase::groupLyrics(const std::vector<LangCore::G2pRes> &input) {
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
    PinyinG2pTaskImplBase::start(const LangCore::NO<LangCore::TaskInput> &input) {
        {
            std::shared_lock lock(m_mutex);
            if (!isEngineInitialized())
                return LangCore::Error(LangCore::Error::RuntimeError,
                                       stdc::formatN("%1Task: chinese g2p not initialized", m_langConfig.languageName));
        }

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        std::vector<LangCore::G2pRes> res;
        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        const auto verifyRes = m_verifier->verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, error] : verifyRes) {
            LangCore::G2pErrorType wordErrorType = error ? LangCore::InvalidLyric : LangCore::NoError;
            res.emplace_back(LangCore::G2pRes{std::string(lyric), std::string(m_spec->id()), std::string(), std::string(),
                                              std::vector<std::string>(), std::string(mode), wordErrorType});
        }

        const auto groupLyric = groupLyrics(res);

        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();

        for (const auto &g2pResGroup : groupLyric) {
            const auto mode = g2pResGroup.front().mode;
            std::vector<std::string> _input;
            for (const auto &g2pRes : g2pResGroup)
                _input.push_back(g2pRes.lyric);

            if (mode != "convert") {
                for (const auto &word : _input) {
                    LangCore::G2pRes newRes;
                    newRes.lyric = word;
                    newRes.g2pId = std::string(m_spec->id());
                    newRes.pronunciation = word;
                    newRes.candidates = std::vector<std::string>();
                    newRes.mode = std::string(mode);
                    newRes.errorType = LangCore::NoError;
                    g2pResult->g2pResult.emplace_back(newRes);
                }
                continue;
            }

            std::vector<Pinyin::PinyinRes> pinyinRes;
            try {
                pinyinRes = doHanziToPinyin(_input);
            } catch (const std::exception &e) {
                for (const auto &word : _input) {
                    LangCore::G2pRes newRes;
                    newRes.lyric = word;
                    newRes.g2pId = std::string(m_spec->id());
                    newRes.pronunciation = word;
                    newRes.candidates = std::vector<std::string>();
                    newRes.mode = std::string("copy");
                    newRes.errorType = LangCore::UnknownError;
                    g2pResult->g2pResult.emplace_back(newRes);
                }
                continue;
            }

            for (auto &[hanzi, pinyin, candidates, conversionError] : pinyinRes) {
                LangCore::G2pErrorType wordErrorType = LangCore::NoError;
                if (conversionError) {
                    wordErrorType = LangCore::InvalidLyric;
                }
                LangCore::G2pRes newRes;
                newRes.lyric = std::string(hanzi);
                newRes.g2pId = std::string(m_spec->id());
                newRes.pronunciation = std::string(pinyin);
                newRes.candidates = std::vector<std::string>();
                newRes.mode = std::string(mode);
                newRes.errorType = wordErrorType;
                g2pResult->g2pResult.emplace_back(newRes);
            }
        }

        m_result = g2pResult;
        return g2pResult;
    }

    std::string PinyinG2pTaskImplBase::getConfig() const {
        if (!m_config.empty()) {
            return m_config;
        }

        LangCore::JsonObject configObj;

        LangCore::JsonObject configuration;
        configuration[m_langConfig.dictPathKey] = LangCore::JsonValue(m_dictPath.string());

        configObj["configuration"] = LangCore::JsonValue(configuration);

        auto json = LangCore::JsonValue(configObj).toJson(2);

        return json;
    }

} // namespace LangPlugins::Common