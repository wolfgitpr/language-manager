#include "TaskImplBase.h"

#include <fstream>
#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Support/Logging.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/TaskPlugin.h>
#include <LangCore/Task/G2pTask.h>

namespace LangPlugins::LstmG2p::Internal
{
    // Helper function to load phoneme mapping from JSON file
    LangCore::Expected<std::map<std::string, int>>
    LstmG2pTaskImplBase::loadPhonemeMapping(const std::filesystem::path &path, const std::string &fieldName) {
        std::map<std::string, int> out;

        std::ifstream file(path);
        if (!file.is_open()) {
            return LangCore::Error(
                LangCore::Error::FileSystemError,
                stdc::formatN(R"(error loading "%1": %2 file not found)", fieldName, stdc::path::to_utf8(path)));
        }

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        std::string buffer(size, '\0');
        file.seekg(0);
        file.read(buffer.data(), size);

        std::string errString;
        const auto j = LangCore::JsonValue::fromJson(buffer, true, &errString);
        if (!errString.empty()) {
            return LangCore::Error(LangCore::Error::ConfigError, errString);
        }

        if (!j.isObject()) {
            return LangCore::Error(LangCore::Error::ConfigError,
                                   stdc::formatN(R"(error loading "%1": outer JSON is not an object)", fieldName));
        }

        const auto &obj = j.toObject();
        for (const auto &[key, value] : obj) {
            if (!value.isInt()) {
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    stdc::formatN(R"(error loading "%1": value of key "%2" is not int)", fieldName, key));
            }
            out[key] = static_cast<int>(value.toInt());
        }

        return out;
    }

    LstmG2pTaskImplBase::LstmG2pTaskImplBase(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> LstmG2pTaskImplBase::initialize() {
        std::unique_lock lock(m_mutex);

        static LangCore::LogCategory Log("lstmG2p");

        // Get driver from package manager (graceful degradation)
        bool driverFound = false;
        auto driverCate = m_spec->Mgr()->category("driver");
        if (driverCate) {
            auto driverObj = driverCate->getFirstObject("g2pOnnxDriver");
            if (driverObj) {
                m_driver = driverObj.as<LangCore::SessionFactory>();
                driverFound = true;
            }
        }

        if (!driverFound) {
            Log.langCoreWarning("ONNX driver unavailable: inference will be disabled for module '%1'. "
                                "Words will be returned as-is with DriverUnavailable error.", m_spec->id());
        }

        auto cfg = LangCore::config(m_spec);

        // Required fields - 存储到私有成员变量
        auto encoderExp = cfg.getPath("encoder");
        if (!encoderExp) {
            return encoderExp.takeError();
        }
        m_encoderPath = encoderExp.take();

        auto decoderExp = cfg.getPath("decoder");
        if (!decoderExp) {
            return decoderExp.takeError();
        }
        m_decoderPath = decoderExp.take();

        // Load charVocab
        auto charVocabPathExp = cfg.getPath("charVocab");
        if (!charVocabPathExp) {
            return charVocabPathExp.takeError();
        }
        m_charVocabPath = charVocabPathExp.take();
        auto charVocabMapping = loadPhonemeMapping(m_charVocabPath, "charVocab");
        if (!charVocabMapping) {
            return charVocabMapping.takeError();
        }
        m_charVocab = charVocabMapping.take();

        // Load phonemeVocab
        auto phonemeVocabPathExp = cfg.getPath("phonemeVocab");
        if (!phonemeVocabPathExp) {
            return phonemeVocabPathExp.takeError();
        }
        m_phonemeVocabPath = phonemeVocabPathExp.take();
        auto phonemeVocabMapping = loadPhonemeMapping(m_phonemeVocabPath, "phonemeVocab");
        if (!phonemeVocabMapping) {
            return phonemeVocabMapping.takeError();
        }
        m_phonemeVocab = phonemeVocabMapping.take();

        for (const auto &[phoneme, index] : m_phonemeVocab)
            m_idxToPhoneme[index] = phoneme;

        // Only open sessions if driver is available
        if (!driverFound) {
            m_driverAvailable = false;
            return {};
        }

        m_encoderSession = m_driver->createSession();
        const auto encoderOpenArgs = LangCore::NO<LangCore::SessionOpenArgs>::create();
        encoderOpenArgs->useCpu = false;
        if (auto res = m_encoderSession->open(m_encoderPath, encoderOpenArgs); !res)
            return res;

        m_decodeSession = m_driver->createSession();
        const auto predictorOpenArgs = LangCore::NO<LangCore::SessionOpenArgs>::create();
        predictorOpenArgs->useCpu = false;
        if (auto res = m_decodeSession->open(m_decoderPath, predictorOpenArgs); !res)
            return res;

        m_driverAvailable = true;
        return {};
    }

    std::string LstmG2pTaskImplBase::getConfig() const {
        std::shared_lock lock(m_mutex);

        // 返回缓存的配置
        if (!m_config.empty()) {
            return m_config;
        }

        // 从私有成员变量生成配置 JSON
        LangCore::JsonObject configObj;

        // 添加 configuration 对象
        LangCore::JsonObject configuration;
        configuration["encoder"] = LangCore::JsonValue(m_encoderPath.string());
        configuration["decoder"] = LangCore::JsonValue(m_decoderPath.string());
        configuration["charVocab"] = LangCore::JsonValue(m_charVocabPath.string());
        configuration["phonemeVocab"] = LangCore::JsonValue(m_phonemeVocabPath.string());

        configObj["configuration"] = LangCore::JsonValue(configuration);

        // 生成 JSON 字符串
        auto json = LangCore::JsonValue(configObj).toJson(2);

        return json;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    LstmG2pTaskImplBase::makeFallbackResult(const std::vector<std::string> &lyrics) const {
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
        g2pResult->g2pResult.reserve(lyrics.size());
        for (const auto &lyric : lyrics) {
            g2pResult->g2pResult.emplace_back(LangCore::G2pRes{
                std::string(lyric), std::string(m_spec->id()), std::string(lyric),
                std::vector<std::string>(), std::string("copy"),
                LangCore::DriverUnavailable});
        }
        g2pResult->errorMessage = "ONNX driver unavailable, returning original lyrics";
        return g2pResult;
    }

    } // namespace LangPlugins::LstmG2p::Internal