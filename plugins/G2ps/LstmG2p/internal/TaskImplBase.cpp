#include "TaskImplBase.h"

#include <fstream>
#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/TaskPlugin.h>

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

        // Get driver from package manager
        auto driverCate = m_spec->Mgr()->category("driver");
        if (!driverCate) {
            return LangCore::Error(LangCore::Error::RuntimeError, "could not find category: driver");
        }

        auto driverObj = driverCate->getFirstObject("g2pOnnxDriver");
        if (!driverObj) {
            return LangCore::Error(LangCore::Error::RuntimeError, "could not find id: g2pOnnxDriver");
        }
        m_driver = driverObj.as<LangCore::SessionFactory>();

        auto cfg = LangCore::config(m_spec);

        // Required fields
        auto encoderExp = cfg.getPath("encoder");
        if (!encoderExp) {
            return encoderExp.takeError();
        }
        auto encoder = encoderExp.take();

        auto decoderExp = cfg.getPath("decoder");
        if (!decoderExp) {
            return decoderExp.takeError();
        }
        auto decoder = decoderExp.take();

        // Load charVocab
        auto charVocabPathExp = cfg.getPath("charVocab");
        if (!charVocabPathExp) {
            return charVocabPathExp.takeError();
        }
        auto charVocabMapping = loadPhonemeMapping(charVocabPathExp.take(), "charVocab");
        if (!charVocabMapping) {
            return charVocabMapping.takeError();
        }
        m_charVocab = charVocabMapping.take();

        // Load phonemeVocab
        auto phonemeVocabPathExp = cfg.getPath("phonemeVocab");
        if (!phonemeVocabPathExp) {
            return phonemeVocabPathExp.takeError();
        }
        auto phonemeVocabMapping = loadPhonemeMapping(phonemeVocabPathExp.take(), "phonemeVocab");
        if (!phonemeVocabMapping) {
            return phonemeVocabMapping.takeError();
        }
        m_phonemeVocab = phonemeVocabMapping.take();

        for (const auto &[phoneme, index] : m_phonemeVocab)
            m_idxToPhoneme[index] = phoneme;

        m_encoderSession = m_driver->createSession();
        const auto encoderOpenArgs = LangCore::NO<LangCore::SessionOpenArgs>::create();
        encoderOpenArgs->useCpu = false;
        if (auto res = m_encoderSession->open(encoder, encoderOpenArgs); !res)
            return res;

        m_decodeSession = m_driver->createSession();
        const auto predictorOpenArgs = LangCore::NO<LangCore::SessionOpenArgs>::create();
        predictorOpenArgs->useCpu = false;
        if (auto res = m_decodeSession->open(decoder, predictorOpenArgs); !res)
            return res;

        return {};
    }

    std::string LstmG2pTaskImplBase::getConfig() const {
        std::shared_lock lock(m_mutex);
        return m_config;
    }

    LangCore::Expected<void> LstmG2pTaskImplBase::setConfig(const std::string &config) {
        std::unique_lock lock(m_mutex);
        m_config = config;
        return {};
    }

} // namespace LangPlugins::LstmG2p::Internal