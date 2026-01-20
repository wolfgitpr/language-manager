#include "RegexTaggerTask.h"

#include <mutex>
#include <shared_mutex>
#include <utility>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <re2/re2.h>
#include <stdcorelib/console.h>

#include <LangMgr/Module/G2pModule.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Task/G2pTask.h>


namespace LangPlugins
{
    namespace fs = std::filesystem;

    static LangMgr::Expected<LangMgr::NO<Regex::RegexTaggerConfiguration>>
    getConfig(const LangMgr::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangMgr::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "RegexTagger configuration is nullptr");
        if (!(genericConfig->className() == Regex::API_CLASS && genericConfig->objectName() == Regex::API_NAME))
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid RegexTagger configuration");
        return genericConfig.as<Regex::RegexTaggerConfiguration>();
    }

    class TaggerRegex {
    public:
        explicit TaggerRegex(const Api::RegexTagger::L1::TaggerRegexEntry &entry, std::string language) :
            entry_(entry), language_(std::move(language)) {
            RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
            RegexOptions.set_log_errors(true);
            RegexOptions.set_max_mem(8 << 20); // 8MB

            regex_ = std::make_unique<RE2>(mergePatterns(entry.regexes), RegexOptions);
            if (!regex_->ok())
                throw std::runtime_error("Invalid regex: " + regex_->error());
        }
        ~TaggerRegex() = default;

        void tag(std::vector<LangMgr::TaggerRes> &input) {
            std::string pattern = regex_->pattern();
            for (auto &it : input) {
                if (it.language != "unknown")
                    continue;
                const auto isMatched = RE2::FullMatch(it.lyric, *regex_);
                it.language = isMatched ? language_ : "unknown";
                it.tag = isMatched ? entry_.tag : "unknown";
            }
        }

        std::vector<LangMgr::TaggerRes> split(const std::vector<LangMgr::TaggerRes> &input) const {
            std::vector<LangMgr::TaggerRes> result;
            for (const auto &taggerRes : input) {
                std::vector<std::string> _result;
                const auto rawStr = taggerRes.lyric;
                if (rawStr.empty())
                    continue;

                re2::StringPiece text(rawStr.data(), rawStr.size());
                size_t last_end = 0;
                re2::StringPiece match;

                while (RE2::FindAndConsume(&text, *regex_, &match)) {
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
        Api::RegexTagger::L1::TaggerRegexEntry entry_;
        std::string language_;

        static std::string mergePatterns(const std::vector<std::string> &patterns) {
            if (patterns.empty())
                return "";

            std::ostringstream oss;
            oss << "(?:" << patterns[0] << ")";

            for (size_t i = 1; i < patterns.size(); ++i)
                oss << "|(?:" << patterns[i] << ")";

            return oss.str();
        }
    };

    class RegexTaggerTask::Impl {
    public:
        LangMgr::NO<LangMgr::TaggerResult> result;
        RE2::Options RegexOptions;
        std::vector<std::unique_ptr<TaggerRegex>> regexes;
        mutable std::shared_mutex mutex;
    };

    RegexTaggerTask::RegexTaggerTask(const LangMgr::ModuleSpec *spec) :
        Task(spec), _impl(std::make_unique<Impl>()) {}

    RegexTaggerTask::~RegexTaggerTask() = default;

    LangMgr::Expected<void> RegexTaggerTask::initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "RegexTagger task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Regex::API_NAME) {
        //     return LangMgr::Error(LangMgr::Error::InvalidArgument,
        //                           stdc::formatN(R"(invalid RegexTagger task init args name: expected "%1", got
        //                           "%2")",
        //                                         Regex::API_NAME, name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get RegexTagger config
        auto expConfig = getConfig(spec()->as<LangMgr::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        impl.RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        impl.RegexOptions.set_log_errors(true);
        impl.RegexOptions.set_max_mem(8 << 20); // 8MB

        for (const auto &entry : config->regexEntry)
            impl.regexes.push_back(std::make_unique<TaggerRegex>(entry, config->language));

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
    RegexTaggerTask::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        setState(Running);

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangMgr::G2pSpec>()); !expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }

        if (!input) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "g2p input is nullptr");
        }

        // if (const auto &name = input->objectName(); name != Regex::API_NAME) {
        //     setState(Failed);
        //     return LangMgr::Error(
        //         LangMgr::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Regex::API_NAME,
        //         name));
        // }

        const auto taggerInput = input.as<LangMgr::TaggerStartInput>();
        std::vector<LangMgr::TaggerRes> res = taggerInput->taggerInput;
        if (taggerInput->split) {
            res = taggerInput->taggerInput;
            for (const auto &tagger : impl.regexes)
                res = tagger->split(res);
        }

        for (const auto &tagger : impl.regexes)
            tagger->tag(res);

        // Create result
        auto taggerResult = LangMgr::NO<LangMgr::TaggerResult>::create(
            LangMgr::TAGGER_API_NAME, LangMgr::TAGGER_API_CLASS, LangMgr::TAGGER_API_LEVEL);
        taggerResult->taggerResult = res;

        impl.result = taggerResult;
        setState(Idle);
        return taggerResult;
    }

    LangMgr::Expected<void> RegexTaggerTask::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                        const StartAsyncCallback &callback) {
        // TODO:
        return LangMgr::Error(LangMgr::Error::NotImplemented);
    }

    bool RegexTaggerTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangMgr::NO<LangMgr::TaskResult> RegexTaggerTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins
