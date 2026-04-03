#include "TagAndValidateStep.h"
#include <LangCore/Support/Error.h>
#include <LangCore/Support/PhonemeDict.h>
#include <re2/re2.h>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> TagAndValidateStep::configure(const LangCore::ModuleSpec *spec,
                                                            const LangCore::JsonObject &config)
    {
        m_spec = spec;

        // 解析配置 - 支持两种格式
        // 1. TemplateG2p 格式（向后兼容）：config 直接包含 "verify" 字段
        // 2. 责任链格式：config 包含 "tagger" 字段（在 params 内）
        
        std::vector<VerifyEntry> entries;
        
        // 优先查找 tagger 字段（责任链格式）
        auto taggerIt = config.find("tagger");
        if (taggerIt != config.end() && taggerIt->second.isArray()) {
            const auto &taggerArray = taggerIt->second.toArray();
            
            for (const auto &item : taggerArray) {
                if (!item.isObject()) {
                    continue;
                }
                
                const auto &obj = item.toObject();
                VerifyEntry entry;
                
                // 解析 type
                auto typeIt = obj.find("type");
                if (typeIt != obj.end() && typeIt->second.isString()) {
                    entry.type = typeIt->second.toString();
                }
                
                // 解析 value
                auto valueIt = obj.find("value");
                if (valueIt != obj.end() && valueIt->second.isArray()) {
                    for (const auto &v : valueIt->second.toArray()) {
                        if (v.isString()) {
                            entry.value.push_back(v.toString());
                        }
                    }
                }
                
                // 解析 action (责任链格式) 或 mode (TemplateG2p 格式)
                auto actionIt = obj.find("action");
                if (actionIt != obj.end() && actionIt->second.isString()) {
                    entry.mode = actionIt->second.toString();
                } else {
                    auto modeIt = obj.find("mode");
                    if (modeIt != obj.end() && modeIt->second.isString()) {
                        entry.mode = modeIt->second.toString();
                    }
                }
                
                entries.push_back(entry);
            }
        } 
        // 查找 verify 字段（TemplateG2p 格式）
        else {
            auto verifyIt = config.find("verify");
            if (verifyIt == config.end() || !verifyIt->second.isArray()) {
                // 如果没有配置 verify/tagger，使用默认规则
                m_verifyEntries = {
                    {"regex", {"([A-Z]+)"}, "copy"},
                    {"regex", {"([a-z]+)"}, "convert"}
                };
                return {};
            }

            const auto &verifyArray = verifyIt->second.toArray();
            entries.reserve(verifyArray.size());

            for (const auto &item : verifyArray) {
                if (!item.isObject()) {
                    continue;
                }

                const auto &obj = item.toObject();
                VerifyEntry entry;

                auto typeIt = obj.find("type");
                if (typeIt != obj.end() && typeIt->second.isString()) {
                    entry.type = typeIt->second.toString();
                }

                auto valueIt = obj.find("value");
                if (valueIt != obj.end() && valueIt->second.isArray()) {
                    for (const auto &v : valueIt->second.toArray()) {
                        if (v.isString()) {
                            entry.value.push_back(v.toString());
                        }
                    }
                }

                auto modeIt = obj.find("mode");
                if (modeIt != obj.end() && modeIt->second.isString()) {
                    entry.mode = modeIt->second.toString();
                }

                entries.push_back(entry);
            }
        }
        
        m_verifyEntries = entries;
        return {};
    }

    void TagAndValidateStep::handle(G2pContext &context)
    {
        for (auto &word : context.words()) {
            // 优先使用 cleanedLyric，如果为空则使用 lyric
            std::string wordToCheck = word.cleanedLyric.empty() ? word.lyric : word.cleanedLyric;
            std::string mode;
            if (verifyWord(wordToCheck, mode)) {
                word.mode = mode;
            } else {
                word.mode = "copy";  // 默认模式
            }
        }
    }

    bool TagAndValidateStep::verifyWord(const std::string &word, std::string &mode) const
    {
        RE2::Options options;
        options.set_encoding(RE2::Options::EncodingUTF8);

        for (const auto &entry : m_verifyEntries) {
            if (entry.type == "regex") {
                for (const auto &pattern : entry.value) {
                    RE2 regex(pattern, options);
                    if (regex.ok() && RE2::FullMatch(word, regex)) {
                        mode = entry.mode;
                        return true;
                    }
                }
            }
        }

        return false;
    }

} // namespace LangPlugins::ChainG2p