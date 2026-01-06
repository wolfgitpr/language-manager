#ifndef LANGUAGE_MANAGER_LOGGING_H
#define LANGUAGE_MANAGER_LOGGING_H

#include <stdcorelib/str.h>

#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{

    class LogContext {
    public:
        LogContext() noexcept = default;
        LogContext(const char *fileName, const int lineNumber, const char *functionName,
                   const char *categoryName) noexcept :
            line(lineNumber), file(fileName), function(functionName), category(categoryName) {}

        int line = 0;
        const char *file = nullptr;
        const char *function = nullptr;
        const char *category = nullptr;
    };

    class LANGMGR_EXPORT Logger {
    public:
        enum Level {
            Trace = 1,
            Debug,
            Success,
            Information,
            Warning,
            Critical,
            Fatal,
        };

        explicit Logger(LogContext context) : _context(std::move(context)) {}

        Logger(const char *file, const int line, const char *function, const char *category) :
            _context(file, line, function, category) {}

        template <class... Args>
        void trace(const std::string_view &format, Args &&...args) {
            print(Trace, stdc::formatN(format, std::forward<Args>(args)...));
        }

        template <class... Args>
        void debug(const std::string_view &format, Args &&...args) {
            print(Debug, stdc::formatN(format, std::forward<Args>(args)...));
        }

        template <class... Args>
        void success(const std::string_view &format, Args &&...args) {
            print(Success, stdc::formatN(format, std::forward<Args>(args)...));
        }

        template <class... Args>
        void info(const std::string_view &format, Args &&...args) {
            print(Information, stdc::formatN(format, std::forward<Args>(args)...));
        }

        template <class... Args>
        void warning(const std::string_view &format, Args &&...args) {
            print(Warning, stdc::formatN(format, std::forward<Args>(args)...));
        }

        template <class... Args>
        void critical(const std::string_view &format, Args &&...args) {
            print(Critical, stdc::formatN(format, std::forward<Args>(args)...));
        }

        template <class... Args>
        void fatal(const std::string_view &format, Args &&...args) {
            print(Critical, stdc::formatN(format, std::forward<Args>(args)...));
            abort();
        }

        template <class... Args>
        void log(const int level, const std::string_view &format, Args &&...args) {
            print(level, stdc::formatN(format, std::forward<Args>(args)...));
        }

        void print(int level, const std::string_view &message) const;

        void printf(int level, const char *fmt, ...) const;

        static void abort();

    public:
        using LogCallback = void (*)(int, const LogContext &, const std::string_view &);

        static LogCallback logCallback();
        static void setLogCallback(LogCallback callback);

    protected:
        LogContext _context;
    };

    /// Yet another logging category implementation of Qt QLoggingCategory.
    class LANGMGR_EXPORT LogCategory {
    public:
        explicit LogCategory(const char *name);
        ~LogCategory();

        const char *name() const { return _name; }
        bool isLevelEnabled(const int level) const { return levelEnabled[level]; }
        void setLevelEnabled(const int level, const bool enabled) { levelEnabled[level] = enabled; }

        using LogCategoryFilter = void (*)(LogCategory *);

        static LogCategoryFilter logFilter();
        static void setLogFilter(LogCategoryFilter filter);

        static std::string filterRules();
        static void setFilterRules(std::string rules);

        static LogCategory &defaultCategory();

        template <int Level, class... Args>
        void log(const char *fileName, const int lineNumber, const char *functionName, const std::string_view &format,
                 Args &&...args) const {
            if (!isLevelEnabled(Level)) {
                return;
            }
            Logger(fileName, lineNumber, functionName, _name).log(Level, format, std::forward<Args>(args)...);
            if constexpr (Level == Logger::Fatal) {
                Logger::abort();
            }
        }

        template <int Level, class... Args>
        void logf(const char *fileName, const int lineNumber, const char *functionName, const char *fmt,
                  Args &&...args) const {
            if (!isLevelEnabled(Level)) {
                return;
            }
            Logger(fileName, lineNumber, functionName, _name).printf(Level, fmt, std::forward<Args>(args)...);
            if constexpr (Level == Logger::Fatal) {
                Logger::abort();
            }
        }

        const LogCategory &_langMgrGetLogCategory() const { return *this; }

    protected:
        const char *_name;

        union {
            bool levelEnabled[8];
            uint64_t enabled;
        };
    };

} // namespace LangMgr

static const LangMgr::LogCategory &_langMgrGetLogCategory() { return LangMgr::LogCategory::defaultCategory(); }

/*!
    \macro langMgrDebug
    \brief Logs a debug message to a log category.
    \code
        // User category
        langMgr::LogCategory lc("test");
        lc.setLevelEnabled(langMgr::Logger::Debug, true);
        lc.langMgrDebug("This is a debug message");
        lc.langMgrDebug("This is a debug message with arg: %1", 42);
        lc.langMgrDebugF("This is a debug message with arg: %d", 42);

        // Default category
        langMgrDebug("This is a debug message");
        langMgrDebug("This is a debug message with arg: %1", 42);
        langMgrDebug("This is a debug message with arg: %d", 42);
    \endcode
*/

#define langMgrLog(LEVEL, ...)                                                                                         \
    _langMgrGetLogCategory().log<LangMgr::Logger::LEVEL>(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define langMgrTrace(...) langMgrLog(Trace, __VA_ARGS__)
#define langMgrDebug(...) langMgrLog(Debug, __VA_ARGS__)
#define langMgrSuccess(...) langMgrLog(Success, __VA_ARGS__)
#define langMgrInfo(...) langMgrLog(Information, __VA_ARGS__)
#define langMgrWarning(...) langMgrLog(Warning, __VA_ARGS__)
#define langMgrCritical(...) langMgrLog(Critical, __VA_ARGS__)
#define langMgrFatal(...) langMgrLog(Critical, __VA_ARGS__)

#define langMgrLogF(LEVEL, ...)                                                                                        \
    _langMgrGetLogCategory().logf<langMgr::Logger::LEVEL>(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define langMgrTraceF(...) langMgrLogF(Trace, __VA_ARGS__)
#define langMgrDebugF(...) langMgrLogF(Debug, __VA_ARGS__)
#define langMgrSuccessF(...) langMgrLogF(Success, __VA_ARGS__)
#define langMgrInfoF(...) langMgrLogF(Information, __VA_ARGS__)
#define langMgrWarningF(...) langMgrLogF(Warning, __VA_ARGS__)
#define langMgrCriticalF(...) langMgrLogF(Critical, __VA_ARGS__)
#define langMgrFatalF(...) langMgrLogF(Critical, __VA_ARGS__)

#endif // LANGUAGE_MANAGER_LOGGING_H
