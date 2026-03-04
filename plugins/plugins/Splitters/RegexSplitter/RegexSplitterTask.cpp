#include "RegexSplitterTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <re2/re2.h>
#include <stdcorelib/console.h>

#include <LangCore/Module/G2pModule.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>


namespace LangPlugins::RegexSplitter
{
    namespace fs = std::filesystem;

    static LangCore::Expected<LangCore::NO<Regex::RegexSplitterConfiguration>>
    getConfig(const LangCore::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangCore::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangCore::Error(LangCore::Error::InvalidArgument, "RegexSplitter configuration is nullptr.");
        if (!(genericConfig->className() == Regex::API_CLASS && genericConfig->objectName() == Regex::API_NAME))
            return LangCore::Error(LangCore::Error::InvalidArgument, "Invalid RegexSplitter configuration.");
        return genericConfig.as<Regex::RegexSplitterConfiguration>();
    }

    class SplitterRegex {
    public:
        explicit SplitterRegex(const std::string &regex) {
            RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
            RegexOptions.set_log_errors(true);
            RegexOptions.set_max_mem(8 << 20); // 8MB

            regex_ = std::make_unique<RE2>(regex, RegexOptions);
            if (!regex_->ok())
                throw std::runtime_error("Invalid regex: " + regex_->error());
        }
        ~SplitterRegex() = default;

        std::vector<std::string> split(const std::vector<std::string> &input) const {
            std::vector<std::string> result;
            for (const auto &rawStr : input) {
                std::vector<std::string> _result;
                if (rawStr.empty())
                    continue;

                re2::StringPiece text(rawStr.data(), rawStr.size());
                size_t last_end = 0;
                re2::StringPiece match;

                while (RE2::FindAndConsume(&text, *regex_, &match)) {
                    if (match.empty()) {
                        result.emplace_back(rawStr);
                        continue;
                    }
                    const size_t match_start = match.data() - rawStr.data();

                    if (match_start > last_end) {
                        _result.emplace_back(rawStr.data() + last_end, match_start - last_end);
                    }

                    _result.emplace_back(match.data(), match.size());
                    last_end = match_start + match.size();
                }

                if (last_end < rawStr.size())
                    _result.emplace_back(rawStr.data() + last_end, rawStr.size() - last_end);

                if (_result.empty())
                    _result.emplace_back(rawStr);

                for (auto &it : _result)
                    result.emplace_back(it);
            }
            return result;
        }

    private:
        RE2::Options RegexOptions;
        std::unique_ptr<RE2> regex_;
    };

    class RegexSplitterTask::Impl {
    public:
        LangCore::NO<LangCore::SplitterResult> result;
        RE2::Options RegexOptions;
        std::vector<std::unique_ptr<SplitterRegex>> regexes;
        mutable std::shared_mutex mutex;
    };

    RegexSplitterTask::RegexSplitterTask(const LangCore::ModuleSpec *spec) :
        Task(spec), _impl(std::make_unique<Impl>()) {}

    RegexSplitterTask::~RegexSplitterTask() = default;

    LangCore::Expected<void> RegexSplitterTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "RegexSplitter task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Regex::API_NAME) {
        //     return LangCore::Error(LangCore::Error::InvalidArgument,
        //                           stdc::formatN(R"(invalid RegexSplitter task init args name: expected "%1", got
        //                           "%2")",
        //                                         Regex::API_NAME, name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get RegexSplitter config
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        impl.RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        impl.RegexOptions.set_log_errors(true);
        impl.RegexOptions.set_max_mem(8 << 20); // 8MB

        for (const auto &regex : config->regexes)
            impl.regexes.push_back(std::make_unique<SplitterRegex>(regex));

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;
        setState(Running);

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>()); !expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }

        if (!input) {
            setState(Failed);
            return LangCore::Error(LangCore::Error::InvalidArgument, "splitter input is nullptr");
        }

        // if (const auto &name = input->objectName(); name != Regex::API_NAME) {
        //     setState(Failed);
        //     return LangCore::Error(
        //         LangCore::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Regex::API_NAME,
        //         name));
        // }

        const auto splitterInput = input.as<LangCore::SplitterStartInput>();
        std::vector<std::string> res;
        for (const auto &regex : impl.regexes)
            res = regex->split(splitterInput->splitterInput);
        // Create result
        auto taggerResult = LangCore::NO<LangCore::SplitterResult>::create(
            LangCore::SPLITTER_API_NAME, LangCore::SPLITTER_API_CLASS, LangCore::SPLITTER_API_LEVEL);
        taggerResult->splitterResult = res;

        impl.result = taggerResult;
        setState(Idle);
        return taggerResult;
    }

    LangCore::Expected<void> RegexSplitterTask::startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                                           const StartAsyncCallback &callback) {
        // TODO:
        return LangCore::Error(LangCore::Error::NotImplemented);
    }

    bool RegexSplitterTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangCore::NO<LangCore::TaskResult> RegexSplitterTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins::RegexSplitter
