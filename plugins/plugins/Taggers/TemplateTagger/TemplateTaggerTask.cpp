#include "TemplateTaggerTask.h"

#include <mutex>
#include <shared_mutex>
#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <re2/re2.h>

#include <LangCore/Module/G2pModule.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>

#include <LangCore/Task/TaggerTask.h>
#include "InferUtil/ErrorCollector.h"
#include "InferUtil/Parser.h"
#include "TaggerUtil.h"


namespace LangPlugins::TemplateTagger
{
    namespace fs = std::filesystem;

    static void parse_tagger_required(std::vector<TaggerUtilEntry> &out, const std::string &fieldName,
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
                    TaggerUtilEntry entry;

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
                        std::string combined;
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

    class TemplateTaggerTask::Impl {
    public:
        LangCore::NO<LangCore::TaggerResult> result;
        RE2::Options RegexOptions;
        std::unique_ptr<TaggerUtil> taggerUtil;
        mutable std::shared_mutex mutex;
    };

    TemplateTaggerTask::TemplateTaggerTask(const LangCore::ModuleSpec *spec) :
        Task(spec), _impl(std::make_unique<Impl>()) {}

    TemplateTaggerTask::~TemplateTaggerTask() = default;

    int TemplateTaggerTask::apiLevel() const { return 1; }

    LangCore::Expected<void> TemplateTaggerTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "TemplateTagger task init args is nullptr.");
        }

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec(), &ec);

        std::string language;
        std::vector<TaggerUtilEntry> entries;

        parser.parse_string_required(language, "language");
        parse_tagger_required(entries, "tagger", spec());

        impl.RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        impl.RegexOptions.set_log_errors(true);
        impl.RegexOptions.set_max_mem(8 << 20); // 8MB

        auto expVerifier = TaggerUtil::Create(entries, language);
        if (!expVerifier)
            return expVerifier.takeError();
        impl.taggerUtil = expVerifier.take();

        // return success
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateTaggerTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;

        if (!input)
            return LangCore::Error(LangCore::Error::InvalidArgument, "Tagger input is nullptr.");

        const auto taggerInput = input.as<LangCore::TaggerStartInput>();
        std::vector<LangCore::TaggerRes> res = taggerInput->taggerInput;
        impl.taggerUtil->tagger(res);

        // Create result
        auto taggerResult = LangCore::NO<LangCore::TaggerResult>::create(
            LangCore::TAGGER_API_NAME, LangCore::TAGGER_API_CLASS, LangCore::TAGGER_API_LEVEL);
        taggerResult->taggerResult = res;

        impl.result = taggerResult;
        return taggerResult;
    }
} // namespace LangPlugins::TemplateTagger
