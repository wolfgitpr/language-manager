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
#include <LangCore/Support/ConfigAccessor.h>


namespace LangPlugins::RegexSplitter::V1
{
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

    class SplitterRegex {
    public:
        explicit SplitterRegex(const std::string &regex) {
            regex_ = std::make_unique<RE2>(regex, g_utf8RegexOptions);
            if (!regex_->ok())
                throw std::runtime_error("Invalid regex: " + regex_->error());
        }
        ~SplitterRegex() = default;

        std::vector<std::string> split(const std::vector<std::string> &input) const {
            std::vector<std::string> result;
            result.reserve(input.size() * 2);  // 预分配足够空间
            
            for (const auto &rawStr : input) {
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
                        result.emplace_back(rawStr.data() + last_end, match_start - last_end);
                    }

                    result.emplace_back(match.data(), match.size());
                    last_end = match_start + match.size();
                }

                if (last_end < rawStr.size())
                    result.emplace_back(rawStr.data() + last_end, rawStr.size() - last_end);

                if (result.empty())
                    result.emplace_back(rawStr);
            }
            return result;
        }

    private:
        std::unique_ptr<RE2> regex_;
    };

    class RegexSplitterTask::Impl {
    public:
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

        // 使用 ConfigAccessor 获取配置
        auto cfg = LangCore::config(spec());
        
        auto regexes = cfg.getStringArray("regexes");
        if (!regexes) {
            return regexes.takeError();
        }

        impl.regexes.clear();
        impl.regexes.reserve(regexes->size());
        
        for (const auto &regex : *regexes)
            impl.regexes.push_back(std::make_unique<SplitterRegex>(regex));

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        __stdc_impl_t;

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "splitter input is nullptr");

        const auto splitterInput = input.as<LangCore::SplitterInputV1>();
        
        // 修正：链式处理所有regex，每个regex处理前一个的结果
        std::vector<std::string> res = splitterInput->splitterInput;
        for (const auto &regex : impl.regexes) {
            res = regex->split(res);
        }
        
        // Create result using move semantics
        auto taggerResult = LangCore::NO<LangCore::SplitterResultV1>::create();
        taggerResult->splitterResult = std::move(res);

        return taggerResult;
    }
} // namespace LangPlugins::RegexSplitter::V1
