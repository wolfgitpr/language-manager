#include "TemplateCleanerTask.h"

#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Error.h>
#include <LangCore/Support/JSON.h>
#include <LangCore/Support/Logging.h>

#include <algorithm>
#include <cctype>
#include <re2/re2.h>

namespace LangPlugins::TemplateCleaner
{
    namespace Internal
    {
        // 清理操作类型
        enum class CleanOperation {
            ToLowercase,       // 转小写
            ToUppercase,       // 转大写
            RemovePunctuation, // 移除标点
            RemoveNumbers,     // 移除数字
            Trim,              // 去除首尾空白
            CustomRegex        // 自定义正则
        };

        // 清理规则
        struct CleanRule {
            CleanOperation operation;
            std::string pattern;       // 用于自定义正则
            std::string replacement;   // 用于自定义正则替换

            bool operator==(const CleanRule &other) const {
                return operation == other.operation && pattern == other.pattern;
            }
        };

        // V1 实现
        class TaskImpl : public LangCore::VersionedTaskImplBase {
        public:
            explicit TaskImpl(const LangCore::ModuleSpec *spec) : m_spec(spec) {}

            LangCore::Expected<void> initialize() override {
                auto cfg = LangCore::config(m_spec);

                // 解析操作列表
                auto operationsExp = cfg.getStringArray("operations");
                if (!operationsExp) {
                    return operationsExp.takeError();
                }
                auto operations = operationsExp.take();

                // 转换操作
                for (const auto &op : operations) {
                    CleanRule rule;
                    if (op == "lowercase") {
                        rule.operation = CleanOperation::ToLowercase;
                    } else if (op == "uppercase") {
                        rule.operation = CleanOperation::ToUppercase;
                    } else if (op == "remove-punctuation") {
                        rule.operation = CleanOperation::RemovePunctuation;
                    } else if (op == "remove-numbers") {
                        rule.operation = CleanOperation::RemoveNumbers;
                    } else if (op == "trim") {
                        rule.operation = CleanOperation::Trim;
                    } else {
                        return LangCore::Error(LangCore::Error::ConfigError,
                                             "Unknown operation: " + op);
                    }
                    m_rules.push_back(rule);
                }

                return {};
            }

            LangCore::Expected<LangCore::NO<LangCore::TaskResult>> start(
                const LangCore::NO<LangCore::TaskInput> &input) override {
                const auto cleanerInput = input.as<LangCore::CleanerInputV1>();
                if (!cleanerInput) {
                    return LangCore::Error(LangCore::Error::RuntimeError,
                                         "Invalid input type, expected CleanerInputV1");
                }

                auto result = LangCore::NO<LangCore::CleanerResultV1>::create();
                result->cleanerResult.reserve(cleanerInput->cleanerInput.size());

                // 对每个输入字符串应用清理规则
                for (const auto &str : cleanerInput->cleanerInput) {
                    std::string cleaned = str;
                    for (const auto &rule : m_rules) {
                        cleaned = applyRule(cleaned, rule);
                    }
                    result->cleanerResult.push_back(cleaned);
                }

                return result;
            }

            std::string getConfig() const override {
                return "{}";
            }

            LangCore::Expected<void> setConfig(const std::string &config) override {
                return {};
            }

        private:
            const LangCore::ModuleSpec *m_spec;
            std::vector<CleanRule> m_rules;

            std::string applyRule(const std::string &str, const CleanRule &rule) const {
                switch (rule.operation) {
                    case CleanOperation::ToLowercase:
                        return toLowercase(str);
                    case CleanOperation::ToUppercase:
                        return toUppercase(str);
                    case CleanOperation::RemovePunctuation:
                        return removePunctuation(str);
                    case CleanOperation::RemoveNumbers:
                        return removeNumbers(str);
                    case CleanOperation::Trim:
                        return trim(str);
                    case CleanOperation::CustomRegex:
                        return applyRegex(str, rule.pattern, rule.replacement);
                    default:
                        return str;
                }
            }

            static std::string toLowercase(const std::string &str) {
                std::string result = str;
                std::transform(result.begin(), result.end(), result.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                return result;
            }

            static std::string toUppercase(const std::string &str) {
                std::string result = str;
                std::transform(result.begin(), result.end(), result.begin(),
                               [](unsigned char c) { return std::toupper(c); });
                return result;
            }

            static std::string removePunctuation(const std::string &str) {
                std::string result;
                result.reserve(str.size());

                // 常见标点符号
                static const std::string punctuation = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

                for (char c : str) {
                    if (punctuation.find(c) == std::string::npos) {
                        result += c;
                    }
                }
                return result;
            }

            static std::string removeNumbers(const std::string &str) {
                std::string result;
                result.reserve(str.size());
                for (char c : str) {
                    if (!std::isdigit(c)) {
                        result += c;
                    }
                }
                return result;
            }

            static std::string trim(const std::string &str) {
                size_t start = str.find_first_not_of(" \t\n\r");
                if (start == std::string::npos) {
                    return "";
                }

                size_t end = str.find_last_not_of(" \t\n\r");
                return str.substr(start, end - start + 1);
            }

            static std::string applyRegex(const std::string &str, const std::string &pattern,
                                         const std::string &replacement) {
                RE2::Options options;
                options.set_encoding(RE2::Options::EncodingUTF8);
                RE2 re(pattern, options);

                if (!re.ok()) {
                    return str;
                }

                std::string result = str;
                RE2::Replace(&result, re, replacement);
                return result;
            }
        };
    } // namespace Internal

    TemplateCleanerTask::TemplateCleanerTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);
        _manager.setImpl(std::make_unique<Internal::TaskImpl>(spec));
    }

    TemplateCleanerTask::~TemplateCleanerTask() = default;

    int TemplateCleanerTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> TemplateCleanerTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateCleanerTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string TemplateCleanerTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> TemplateCleanerTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }

} // namespace LangPlugins::TemplateCleaner