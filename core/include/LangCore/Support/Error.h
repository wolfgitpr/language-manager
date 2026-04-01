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
            InitializationError // 初始化错误（未初始化、初始化失败等）
        };

        Error() : Error(Success) {}

        explicit Error(int type) : Error(static_cast<Type>(type)) {}

        Error(const Type type) : _type(type), _msg(defaultMessage(type)) {}

        Error(const int type, std::string msg) : _type(type), _msg(std::make_shared<std::string>(std::move(msg))) {}

        Error(const int type, const char *msg) : _type(type), _msg(std::make_shared<std::string>(msg)) {}

        int type() const { return _type; }

        bool ok() const { return _type == Success; }

        const std::string &message() const { return *_msg; }

        const char *what() const { return _msg->c_str(); }

        static Error success() { return Error(Success); }

    protected:
        int _type;
        std::shared_ptr<std::string> _msg;

        LANGCORE_EXPORT static std::shared_ptr<std::string> defaultMessage(int type);
    };

} // namespace LangCore

#endif // LANGCORE_ERROR_H
