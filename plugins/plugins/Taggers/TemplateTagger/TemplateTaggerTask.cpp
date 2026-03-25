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

#include "TaggerUtil.h"


namespace LangPlugins::TemplateTagger
{
    namespace fs = std::filesystem;

    static LangCore::Expected<LangCore::NO<Regex::TemplateTaggerConfiguration>>
    getConfig(const LangCore::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangCore::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangCore::Error(LangCore::Error::InvalidArgument, "TemplateTagger configuration is nullptr.");
        if (!(genericConfig->className() == Regex::API_CLASS && genericConfig->objectName() == Regex::API_NAME))
            return LangCore::Error(LangCore::Error::InvalidArgument, "Invalid TemplateTagger configuration.");
        return genericConfig.as<Regex::TemplateTaggerConfiguration>();
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

    LangCore::Expected<void> TemplateTaggerTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "TemplateTagger task init args is nullptr.");
        }

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get TemplateTagger config
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
        if (!expConfig)
            return expConfig.takeError();
        const auto config = expConfig.take();

        impl.RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        impl.RegexOptions.set_log_errors(true);
        impl.RegexOptions.set_max_mem(8 << 20); // 8MB

        auto expVerifier = TaggerUtil::Create(config->taggerUtilEntry, config->language);
        if (!expVerifier)
            return expVerifier.takeError();
        impl.taggerUtil = expVerifier.take();

        // return success
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateTaggerTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>()); !expConfig)
            return expConfig.takeError();

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
