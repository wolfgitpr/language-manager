#include "Logging.h"

#include <cstdarg>
#include <cstdlib>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>

#include <stdcorelib/console.h>

namespace LangMgr
{

    static void defaultLogCallback(int level, const LogContext &context, const std::string_view &message);

    static void defaultLogCategoryFilter(LogCategory *category);

    class LogRegistry {
    public:
        static inline Logger::LogCallback callback = defaultLogCallback;
        static inline LogCategory::LogCategoryFilter categoryFilter = defaultLogCategoryFilter;

        std::string filterRules;
        std::shared_mutex mutex;

        std::unordered_set<LogCategory *> categories;

        void updateFilterRules() const {
            for (const auto &category : categories) {
                categoryFilter(category);
            }
        }

        static LogRegistry *instance() {
            static LogRegistry registry;
            return &registry;
        }
    };

    static void defaultLogCallback(const int level, const LogContext &context, const std::string_view &message) {
        (void)context;

        if (level < Logger::Success)
            return;

        using namespace stdc;

        auto color = console::nocolor;
        switch (level) {
        case Logger::Success:
            color = console::lightgreen;
            break;
        case Logger::Warning:
            color = console::yellow;
            break;
        case Logger::Critical:
        case Logger::Fatal:
            color = console::red;
            break;
        default:
            break;
        }
        console::puts(console::nostyle, color, console::nocolor, message);
    }

    static void defaultLogCategoryFilter(LogCategory *category) {
        // TODO
    }

    void Logger::print(const int level, const std::string_view &message) const {
        LogRegistry::callback(level, _context, message);
    }

    void Logger::printf(const int level, const char *fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        const std::string message = stdc::vasprintf(fmt, args);
        va_end(args);
        LogRegistry::callback(level, _context, message);
    }

    void Logger::abort() {
        std::abort(); // ###FIXME: robust implementation
    }

    Logger::LogCallback Logger::logCallback() { return LogRegistry::callback; }

    void Logger::setLogCallback(const LogCallback callback) { LogRegistry::callback = callback; }

    LogCategory::LogCategory(const char *name) : _name(name) {
        enabled = 0x0101010101010101ULL;

        auto &reg = *LogRegistry::instance();
        std::unique_lock lock(reg.mutex);
        reg.categories.insert(this);
    }

    LogCategory::~LogCategory() {
        auto &reg = *LogRegistry::instance();
        std::unique_lock lock(reg.mutex);
        reg.categories.erase(this);
    }

    LogCategory::LogCategoryFilter LogCategory::logFilter() {
        auto &reg = *LogRegistry::instance();
        std::shared_lock lock(reg.mutex);
        return LogRegistry::categoryFilter;
    }

    void LogCategory::setLogFilter(LogCategoryFilter filter) {
        auto &reg = *LogRegistry::instance();
        std::unique_lock lock(reg.mutex);

        if (!filter)
            filter = defaultLogCategoryFilter;

        LogRegistry::categoryFilter = filter;
        reg.updateFilterRules();
    }

    std::string LogCategory::filterRules() {
        auto &reg = *LogRegistry::instance();
        std::shared_lock lock(reg.mutex);
        return reg.filterRules;
    }

    void LogCategory::setFilterRules(std::string rules) {
        auto &reg = *LogRegistry::instance();
        std::unique_lock lock(reg.mutex);
        reg.filterRules = std::move(rules);
        reg.updateFilterRules();
    }

    LogCategory &LogCategory::defaultCategory() {
        static LogCategory category("default");
        return category;
    }

} // namespace LangMgr
