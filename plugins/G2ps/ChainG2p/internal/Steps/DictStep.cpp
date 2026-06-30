#include "DictStep.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Error.h>
#include <stdcorelib/path.h>
#include <stdcorelib/str.h>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> DictStep::configure(const LangCore::ModuleSpec *spec,
                                                   const LangCore::JsonObject &config)
    {
        m_spec = spec;

        // 解析 enabled
        auto enabledIt = config.find("enabled");
        if (enabledIt != config.end() && enabledIt->second.isBool()) {
            m_enabled = enabledIt->second.toBool();
        } else {
            m_enabled = true;
        }

        if (!m_enabled) {
            return {};
        }

        // 解析 dictPath - 使用 ConfigAccessor 统一路径规范化
        auto cfg = LangCore::ConfigAccessor(config, spec->path());
        auto dictPathExp = cfg.getResolvedPath("file");
        if (!dictPathExp) {
            return dictPathExp.takeError();
        }
        m_dictPath = dictPathExp.take();

        // 加载字典
        if (m_dictPath.empty()) {
            return LangCore::Error(LangCore::Error::FileSystemError, "Dictionary path is empty");
        }

        std::error_code errorCode;
        if (!m_phonemeDict.load(m_dictPath, &errorCode)) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                 stdc::formatN("Failed to load dictionary: %1 (%2)",
                                               m_dictPath.string(), errorCode.value()));
        }

        return {};
    }

    void DictStep::handle(G2pContext &context)
    {
        if (!m_enabled) {
            return;
        }

        for (auto &word : context.words()) {
            // 只处理需要转换且未丢弃的词
            if (word.mode != "convert" || word.discard) {
                continue;
            }

            // 使用清洗后的词进行查找（如果有）
            std::string lookupKey = word.cleanedLyric.empty() ? word.lyric : word.cleanedLyric;

            // 查找字典
            auto phonemes = lookup(lookupKey);
            if (!phonemes.empty()) {
                // 构建发音字符串
                std::string pronStr;
                for (const auto &phone : phonemes) {
                    pronStr += phone + " ";
                }

                word.pronunciation = pronStr;
                word.candidates = phonemes;
                word.fromDict = true;
            }
        }
    }

    void DictStep::cleanup()
    {
        m_phonemeDict.reset();
    }

    std::vector<std::string> DictStep::lookup(const std::string &key) const
    {
        std::vector<std::string> result;

        if (const auto it = m_phonemeDict.find(key.c_str()); it != m_phonemeDict.end()) {
            const auto &phonemes = it->second;
            // PhonemeList does not have size(), but we can use vec() to convert to vector
            auto phonemeVec = phonemes.vec();
            result.reserve(phonemeVec.size());
            for (const auto &phone : phonemeVec) {
                result.emplace_back(phone);
            }
        }

        return result;
    }

} // namespace LangPlugins::ChainG2p