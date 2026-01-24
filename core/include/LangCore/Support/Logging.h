#ifndef LANGCORE_LOGGING_H
#define LANGCORE_LOGGING_H

#include <stdcorelib/str.h>

#include <LangCore/LangCoreGlobal.h>

namespace LangCore
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

    class LANGCORE_EXPORT Logger {
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
    class LANGCORE_EXPORT LogCategory {
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

        const LogCategory &_langCoreGetLogCategory() const { return *this; }

    protected:
        const char *_name;

        union {
            bool levelEnabled[8];
            uint64_t enabled;
        };
    };

} // namespace LangCore

static const LangCore::LogCategory &_langCoreGetLogCategory() { return LangCore::LogCategory::defaultCategory(); }

/*!
    \macro langCoreDebug
    \brief Logs a debug message to a log category.
    \code
        // User category
        langCore::LogCategory lc("test");
        lc.setLevelEnabled(langCore::Logger::Debug, true);
        lc.langCoreDebug("This is a debug message");
        lc.langCoreDebug("This is a debug message with arg: %1", 42);
        lc.langCoreDebugF("This is a debug message with arg: %d", 42);

        // Default category
        langCoreDebug("This is a debug message");
        langCoreDebug("This is a debug message with arg: %1", 42);
        langCoreDebug("This is a debug message with arg: %d", 42);
    \endcode
*/

#define langCoreLog(LEVEL, ...)                                                                                        \
    _langCoreGetLogCategory().log<LangCore::Logger::LEVEL>(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define langCoreTrace(...) langCoreLog(Trace, __VA_ARGS__)
#define langCoreDebug(...) langCoreLog(Debug, __VA_ARGS__)
#define langCoreSuccess(...) langCoreLog(Success, __VA_ARGS__)
#define langCoreInfo(...) langCoreLog(Information, __VA_ARGS__)
#define langCoreWarning(...) langCoreLog(Warning, __VA_ARGS__)
#define langCoreCritical(...) langCoreLog(Critical, __VA_ARGS__)
#define langCoreFatal(...) langCoreLog(Critical, __VA_ARGS__)

#define langCoreLogF(LEVEL, ...)                                                                                       \
    _langCoreGetLogCategory().logf<langCore::Logger::LEVEL>(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define langCoreTraceF(...) langCoreLogF(Trace, __VA_ARGS__)
#define langCoreDebugF(...) langCoreLogF(Debug, __VA_ARGS__)
#define langCoreSuccessF(...) langCoreLogF(Success, __VA_ARGS__)
#define langCoreInfoF(...) langCoreLogF(Information, __VA_ARGS__)
#define langCoreWarningF(...) langCoreLogF(Warning, __VA_ARGS__)
#define langCoreCriticalF(...) langCoreLogF(Critical, __VA_ARGS__)
#define langCoreFatalF(...) langCoreLogF(Critical, __VA_ARGS__)

#endif // LANGCORE_LOGGING_H
