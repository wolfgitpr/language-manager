#ifndef LANGCORE_ERROR_H
#define LANGCORE_ERROR_H

#include <memory>
#include <string>

#include <LangCore/LangCoreGlobal.h>

namespace LangCore
{

    class Error {
    public:
        enum Type {
            Success = 0,
            ConfigError, // 配置错误（JSON 格式错误、参数错误等）
            FileSystemError, // 文件系统错误（文件未找到、无法打开、重复加载等）
            DependencyError, // 依赖错误（循环依赖、依赖未找到、解释器未找到等）
            RuntimeError, // 运行时错误（会话错误、任务错误、运行时异常等）
            NotImplementedError, // 未实现错误（功能不支持、方法未实现等）
            InitializationError, // 初始化错误（未初始化、初始化失败等）
            ValidationError, // 验证错误（参数验证失败、数据验证失败等）
            NullPointerError, // 空指针错误（nullptr 访问、无效指针等）
            IndexError, // 索引错误（数组越界、无效索引等）
            TimeoutError // 超时错误（操作超时、响应超时等）
        };

        /// 错误上下文信息
        struct Context {
            std::string file;     // 源文件名
            int line;             // 行号
            std::string function; // 函数名
            std::string extra;    // 额外信息

            Context() : line(0) {}
        };

        Error() : Error(Success) {}

        Error(const Type type) : _type(type), _msg(defaultMessage(type)) {}

        Error(const Type type, std::string msg) : _type(type), _msg(std::make_shared<std::string>(std::move(msg))) {}

        Error(const Type type, const char *msg) : _type(type), _msg(std::make_shared<std::string>(msg)) {}

        Error(const Type type, std::string msg, std::string suggestion)
            : _type(type), _msg(std::make_shared<std::string>(std::move(msg))),
              _suggestion(std::make_shared<std::string>(std::move(suggestion))) {}

        Error(const Type type, const char *msg, const char *suggestion)
            : _type(type), _msg(std::make_shared<std::string>(msg)),
              _suggestion(std::make_shared<std::string>(suggestion)) {}

        Type type() const { return _type; }

        bool ok() const { return _type == Success; }

        const std::string &message() const { return *_msg; }

        const char *what() const { return _msg->c_str(); }

        const std::string &suggestion() const {
            static const std::string emptySuggestion;
            return _suggestion ? *_suggestion : emptySuggestion;
        }

        bool hasSuggestion() const { return _suggestion != nullptr; }

        /// 获取错误上下文
        const Context &context() const {
            static const Context emptyContext;
            return _context ? *_context : emptyContext;
        }

        /// 检查是否有上下文信息
        bool hasContext() const { return _context != nullptr; }

        /// 设置错误上下文
        void setContext(const Context &ctx) {
            if (!_context) {
                _context = std::make_shared<Context>();
            }
            *_context = ctx;
        }

        /// 添加错误上下文（链式调用）
        Error &withContext(const std::string &file, int line, const std::string &function) {
            if (!_context) {
                _context = std::make_shared<Context>();
            }
            _context->file = file;
            _context->line = line;
            _context->function = function;
            return *this;
        }

        /// 添加额外信息（链式调用）
        Error &withExtra(const std::string &extra) {
            if (!_context) {
                _context = std::make_shared<Context>();
            }
            _context->extra = extra;
            return *this;
        }

        /// 获取完整的错误信息（包含上下文）
        std::string fullMessage() const {
            std::string result = message();
            if (hasContext()) {
                const auto &ctx = context();
                if (!ctx.file.empty() || ctx.line > 0) {
                    result += "\n  at ";
                    if (!ctx.file.empty()) {
                        result += ctx.file;
                    }
                    if (ctx.line > 0) {
                        result += ":" + std::to_string(ctx.line);
                    }
                    if (!ctx.function.empty()) {
                        result += " (" + ctx.function + ")";
                    }
                }
                if (!ctx.extra.empty()) {
                    result += "\n  extra: " + ctx.extra;
                }
            }
            if (hasSuggestion()) {
                result += "\n  suggestion: " + suggestion();
            }
            return result;
        }

        static Error success() { return Error(Success); }

    protected:
        Type _type;
        std::shared_ptr<std::string> _msg;
        std::shared_ptr<std::string> _suggestion;
        std::shared_ptr<Context> _context;

        LANGCORE_EXPORT static std::shared_ptr<std::string> defaultMessage(Type type);
    };

} // namespace LangCore

#endif // LANGCORE_ERROR_H
