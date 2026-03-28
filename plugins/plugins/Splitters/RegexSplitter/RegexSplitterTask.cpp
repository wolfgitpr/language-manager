#include "RegexSplitterTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <re2/re2.h>
#include <stdcorelib/console.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/SplitterTask.h>

#include <InferUtil/ErrorCollector.h>
#include <InferUtil/Parser.h>


namespace LangPlugins::RegexSplitter
{
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

    int RegexSplitterTask::apiLevel() const { return 1; }

    LangCore::Expected<void> RegexSplitterTask::initialize() {
        __stdc_impl_t;

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec(), &ec);

        std::vector<std::string> regexes;
        parser.parse_stringVec_required(regexes, "regexes");

        impl.RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        impl.RegexOptions.set_log_errors(true);
        impl.RegexOptions.set_max_mem(8 << 20); // 8MB

        for (const auto &regex : regexes)
            impl.regexes.push_back(std::make_unique<SplitterRegex>(regex));

        // return success
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;

        if (!input)
            return LangCore::Error(LangCore::Error::InvalidArgument, "splitter input is nullptr");

        const auto splitterInput = input.as<LangCore::SplitterStartInput>();
        std::vector<std::string> res;
        for (const auto &regex : impl.regexes)
            res = regex->split(splitterInput->splitterInput);
        // Create result
        auto taggerResult = LangCore::NO<LangCore::SplitterResult>::create(
            LangCore::SPLITTER_API_NAME, LangCore::SPLITTER_API_CLASS, LangCore::SPLITTER_API_LEVEL);
        taggerResult->splitterResult = res;

        impl.result = taggerResult;
        return taggerResult;
    }
} // namespace LangPlugins::RegexSplitter
