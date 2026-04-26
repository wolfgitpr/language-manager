#include "FallbackStep.h"
#include <LangCore/Support/ConfigAccessor.h>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> FallbackStep::configure(const LangCore::ModuleSpec *spec,
                                                      const LangCore::JsonObject &config)
    {
        m_spec = spec;

        auto useOriginalIt = config.find("useOriginal");
        m_useOriginal = (useOriginalIt != config.end() && useOriginalIt->second.isBool()) ? useOriginalIt->second.toBool() : true;

        auto defaultPronIt = config.find("defaultPronunciation");
        if (defaultPronIt != config.end() && defaultPronIt->second.isString()) {
            m_defaultPronunciation = defaultPronIt->second.toString();
        }

        auto markFailedIt = config.find("markFailed");
        m_markFailed = (markFailedIt != config.end() && markFailedIt->second.isBool()) ? markFailedIt->second.toBool() : true;

        return {};
    }

    void FallbackStep::handle(G2pContext &context)
    {
        for (auto &word : context.words()) {
            // 跳过已丢弃或已完成的词
            if (word.discard || word.mode == "copy") {
                continue;
            }

            // 如果发音为空，使用回退策略
            if (word.pronunciation.empty()) {
                if (m_useOriginal) {
                    word.pronunciation = word.lyric;
                    word.candidates = {word.lyric};
                } else if (!m_defaultPronunciation.empty()) {
                    word.pronunciation = m_defaultPronunciation;
                    word.candidates = {m_defaultPronunciation};
                }

                word.fromFallback = true;
                word.errorType = LangCore::PhonemeGenerationFailed;
            }
        }
    }

} // namespace LangPlugins::ChainG2p