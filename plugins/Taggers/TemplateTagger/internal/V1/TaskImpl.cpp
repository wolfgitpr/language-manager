#include "TaskImpl.h"

#include <mutex>
#include <shared_mutex>
#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <re2/re2.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/TaggerTask.h>
#include <LangCore/Support/ConfigAccessor.h>

#include "../../TaggerUtil.h"


namespace LangPlugins::TemplateTagger::Internal::V1
{
    namespace fs = std::filesystem;

    namespace {
        // 提取重复的RE2配置为常量
        static const RE2::Options g_utf8RegexOptions = []() {
            RE2::Options options;
            options.set_encoding(RE2::Options::EncodingUTF8);
            options.set_log_errors(true);
            options.set_max_mem(8 << 20); // 8MB
            return options;
        }();
    }

    static void parse_tagger_required(std::vector<LangPlugins::TemplateTagger::V1::TaggerUtilEntry> &out,
                                      const std::string &fieldName,
                                      const LangCore::ModuleSpec *spec) {
        const auto &config = spec->manifestConfiguration();

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (!it->second.isArray()) {
                std::cout << ("array field \"" + fieldName + "\" type mismatch");
            } else {
                const auto &arr = it->second.toArray();
                out.clear();
                out.reserve(arr.size());

                for (size_t i = 0; i < arr.size(); ++i) {
                    const auto &item = arr[i];
                    if (!item.isObject()) {
                        std::cout << ("tagger entry #" + std::to_string(i) + " must be an object");
                        continue;
                    }

                    const auto &obj = item.toObject();
                    LangPlugins::TemplateTagger::V1::TaggerUtilEntry entry;

                    if (const auto typeIt = obj.find("type"); typeIt != obj.end()) {
                        if (typeIt->second.isString()) {
                            entry.type = typeIt->second.toString();
                        } else {
                            std::cout << ("tagger entry #" + std::to_string(i) + " field \"type\" must be string");
                            continue;
                        }
                    } else {
                        std::cout << ("tagger entry #" + std::to_string(i) + " missing required field \"type\"");
                        continue;
                    }

                    if (const auto typeIt = obj.find("tag"); typeIt != obj.end()) {
                        if (typeIt->second.isString()) {
                            entry.tag = typeIt->second.toString();
                        } else {
                            std::cout << ("tagger entry #" + std::to_string(i) + " field \"tag\" must be string");
                            continue;
                        }
                    } else {
                        std::cout << ("tagger entry #" + std::to_string(i) + " missing required field \"tag\"");
                        continue;
                    }

                    if (const auto valueIt = obj.find("value"); valueIt != obj.end()) {
                        const auto &valueArr = valueIt->second.toArray();
                        entry.value.reserve(valueArr.size());
                        for (size_t j = 0; j < valueArr.size(); ++j) {
                            if (valueArr[j].isString()) {
                                if (entry.type == "dict") {
                                    const auto path = spec->path() / stdc::path::from_utf8(valueArr[j].toString());
                                    entry.value.push_back(path.string());
                                } else
                                    entry.value.push_back(valueArr[j].toString());
                            } else
                                std::cout << ("verify entry #" + std::to_string(i) + " array value #" +
                                              std::to_string(j) + " must be string")
                                          << std::endl;
                        }
                    } else {
                        std::cout << ("tagger entry #" + std::to_string(i) + " missing required field \"value\"")
                                  << std::endl;
                        continue;
                    }

                    if (const auto modeIt = obj.find("discard"); modeIt != obj.end()) {
                        if (modeIt->second.isBool()) {
                            entry.discard = modeIt->second.toBool();
                        } else {
                            std::cout << ("tagger entry #" + std::to_string(i) + " field \"discard\" must be string")
                                      << std::endl;
                            continue;
                        }
                    } else {
                        std::cout << ("tagger entry #" + std::to_string(i) + " missing required field \"mode\"")
                                  << std::endl;
                        continue;
                    }
                    out.push_back(std::move(entry));
                }
            }
        } else {
            std::cout << ("array field \"" + fieldName + "\" is missing") << std::endl;
        }
    }

    TemplateTaggerTaskImpl::TemplateTaggerTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> TemplateTaggerTaskImpl::initialize() {
        std::unique_lock lock(m_mutex);

        auto cfg = LangCore::config(m_spec);

        // Required fields
        auto languageExp = cfg.getString("language");
        if (!languageExp) {
            return languageExp.takeError();
        }
        auto language = languageExp.take();

        std::vector<LangPlugins::TemplateTagger::V1::TaggerUtilEntry> entries;
        parse_tagger_required(entries, "tagger", m_spec);

        auto expVerifier = LangPlugins::TemplateTagger::V1::TaggerUtil::Create(entries, language);
        if (!expVerifier)
            return expVerifier.takeError();
        m_taggerUtil = expVerifier.take();

        // Save configuration
        m_config = LangCore::JsonValue(cfg.raw()).toJson();

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateTaggerTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "Tagger input is nullptr.");

        const auto taggerInput = input.as<LangCore::TaggerInputV1>();
        std::vector<LangCore::TaggerRes> res = taggerInput->taggerInput;
        m_taggerUtil->tagger(res);

        auto taggerResult = LangCore::NO<LangCore::TaggerResultV1>::create();
        taggerResult->taggerResult = res;

        return taggerResult;
    }

    std::string TemplateTaggerTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> TemplateTaggerTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }
} // namespace LangPlugins::TemplateTagger::Internal::V1
