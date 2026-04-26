#include "TaskImpl.h"

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
#include <cpp-pinyin/Jyutping.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::CantoneseG2p::Internal::V1
{
    using namespace LangPlugins::InferUtil;
    CantoneseG2pTaskImpl::CantoneseG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> CantoneseG2pTaskImpl::initialize() {
        std::unique_lock lock(m_mutex);

        m_result.reset();

        auto cfg = LangCore::config(m_spec);

        // Parse verify entries and create verifier
        auto verifyEntryExp = ParseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }

        auto verifierExp = Verifier::Create(verifyEntryExp.take());
        if (!verifierExp) {
            return verifierExp.takeError();
        }
        m_verifier = verifierExp.take();

        // Required fields - 存储到私有成员变量
        auto dictPathExp = cfg.getPath("dictPath");
        if (!dictPathExp) {
            return dictPathExp.takeError();
        }
        m_dictPath = dictPathExp.take();

        Pinyin::setDictionaryPath(m_dictPath);
        m_cantonese = std::make_unique<Pinyin::Jyutping>();

        // 缓存配置 JSON（m_dictPath 已确定，后续不再变化）
        m_config = getConfig();

        if (!m_cantonese->initialized())
            return LangCore::Error(LangCore::Error::InitializationError,
                                   "CantoneseG2p: cpp-pinyin library failed to initialize");

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

        // 使用 m_verifier 进行验证
        const auto verifyRes = m_verifier->verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, error] : verifyRes) {
            LangCore::G2pErrorType wordErrorType = error ? LangCore::InvalidLyric : LangCore::NoError;
            res.emplace_back(LangCore::G2pRes{
                std::string(lyric), std::string(m_spec->id()), std::string(), std::vector<std::string>(), std::string(mode), wordErrorType});
        }

        const auto groupLyric = groupLyrics(res);

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();

        for (const auto &g2pResGroup : groupLyric) {
            const auto mode = g2pResGroup.front().mode;
            std::vector<std::string> _input;
            for (const auto &g2pRes : g2pResGroup)
                _input.push_back(g2pRes.lyric);

            // §14.11 fix: skip hanziToPinyin for "copy" mode words
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

            // §14.26 fix: wrap third-party cpp-pinyin call in try-catch
            std::vector<Pinyin::PinyinRes> pinyinRes;
            try {
                pinyinRes = m_cantonese->hanziToPinyin(_input, Pinyin::CanTone::NORMAL, Pinyin::Default, true);
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

    std::string CantoneseG2pTaskImpl::getConfig() const {
        // 返回缓存的配置
        if (!m_config.empty()) {
            return m_config;
        }

        // 从私有成员变量生成配置 JSON
        LangCore::JsonObject configObj;

        // 添加 configuration 对象
        LangCore::JsonObject configuration;
        configuration["dictPath"] = LangCore::JsonValue(m_dictPath.string());

        configObj["configuration"] = LangCore::JsonValue(configuration);

        // 生成 JSON 字符串
        auto json = LangCore::JsonValue(configObj).toJson(2);

        return json;
    }
} // namespace LangPlugins::CantoneseG2p::Internal::V1