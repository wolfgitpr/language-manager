#include "RegexSplitterTask.h"

#include <mutex>
#include <shared_mutex>

#include <re2/re2.h>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <../../../../language-manager/include/LangMgr/Task/Task.h>
#include <LangPlugins/Core/Tensor.h>

namespace LangPlugins
{
    static LangMgr::Expected<LangMgr::NO<Regex::RegexSplitterConfiguration>>
    getConfig(const LangMgr::G2pDefinition *spec) {

        const auto genericConfig = spec->configuration();
        if (!genericConfig) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "RegexSplitter configuration is nullptr");
        }
        if (!(genericConfig->className() == Regex::API_CLASS && genericConfig->objectName() == Regex::API_NAME)) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid RegexSplitter configuration");
        }
        return genericConfig.as<Regex::RegexSplitterConfiguration>();
    }

    class RegexSplitterTask::Impl {
    public:
        LangMgr::NO<Regex::RegexSplitterResult> result;
        RE2::Options RegexOptions;
        std::unique_ptr<RE2> regex_;
        mutable std::shared_mutex mutex;
    };

    RegexSplitterTask::RegexSplitterTask(const LangMgr::ModuleDefinition *definition) :
        Task(definition), _impl(std::make_unique<Impl>()) {}

    RegexSplitterTask::~RegexSplitterTask() = default;

    LangMgr::Expected<void> RegexSplitterTask::initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "RegexSplitter task init args is nullptr");
        }
        if (auto name = args->objectName(); name != Regex::API_NAME) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN(R"(invalid RegexSplitter task init args name: expected "%1", got "%2")",
                                                Regex::API_NAME, name));
        }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get RegexSplitter config
        auto expConfig = getConfig(spec()->as<LangMgr::G2pDefinition>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        impl.RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        impl.RegexOptions.set_log_errors(true);
        impl.RegexOptions.set_max_mem(8 << 20); // 8MB

        impl.regex_ = std::make_unique<RE2>(config->regexStr, impl.RegexOptions);
        if (!impl.regex_->ok())
            throw std::runtime_error("Invalid regex: " + impl.regex_->error());

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
    RegexSplitterTask::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        setState(Running);

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangMgr::G2pDefinition>()); !expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }

        if (!input) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "spliter input is nullptr");
        }

        if (const auto &name = input->objectName(); name != Regex::API_NAME) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN(R"(invalid spliter task init args name: expected "%1", got "%2")",
                                                Regex::API_NAME, name));
        }

        const auto spliterInput = input.as<Regex::RegexSplitterStartInput>();

        // Preprocess input word
        if (spliterInput->rawStrVec.empty()) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "input words are empty");
        }

        const auto regexResult = LangMgr::NO<Regex::RegexSplitterResult>::create();

        for (auto rawStr : spliterInput->rawStrVec) {
            std::vector<std::string> result;

            if (rawStr.empty())
                continue;

            re2::StringPiece text(rawStr.data(), rawStr.size());
            size_t last_end = 0;
            re2::StringPiece match;

            result.reserve(32);

            while (RE2::FindAndConsume(&text, *impl.regex_, &match)) {
                const size_t match_start = match.data() - rawStr.data();

                if (match_start > last_end) {
                    result.emplace_back(rawStr.data() + last_end, match_start - last_end);
                }

                result.emplace_back(match.data(), match.size());
                last_end = match_start + match.size();
            }

            if (last_end < rawStr.size())
                result.emplace_back(rawStr.data() + last_end, rawStr.size() - last_end);

            if (result.empty())
                result.emplace_back(rawStr);
            regexResult->resStrVec.insert(regexResult->resStrVec.end(), std::move_iterator(result.begin()),
                                          std::move_iterator(result.end()));
        }

        impl.result = regexResult;
        setState(Idle);
        return impl.result;
    }

    LangMgr::Expected<void> RegexSplitterTask::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                          const StartAsyncCallback &callback) {
        // TODO:
        return LangMgr::Error(LangMgr::Error::NotImplemented);
    }

    bool RegexSplitterTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangMgr::NO<LangMgr::TaskResult> RegexSplitterTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }


} // namespace LangPlugins
