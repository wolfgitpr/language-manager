#ifndef LANGUAGE_MANAGER_ERROR_H
#define LANGUAGE_MANAGER_ERROR_H

#include <memory>
#include <string>

#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{

    class Error {
    public:
        enum Type {
            NoError = 0,
            InvalidFormat,
            FileNotFound,
            FileNotOpen,
            FileDuplicated,
            RecursiveDependency,
            FeatureNotSupported,
            InvalidArgument,
            NotImplemented,
            SessionError,
            InterpreterNotFound,
        };

        Error() : Error(NoError) {}

        explicit Error(int type) : Error(static_cast<Type>(type)) {}

        Error(const Type type) : _type(type), _msg(defaultMessage(type)) {}

        Error(const int type, std::string msg) : _type(type), _msg(std::make_shared<std::string>(std::move(msg))) {}

        Error(const int type, const char *msg) : _type(type), _msg(std::make_shared<std::string>(msg)) {}

        int type() const { return _type; }

        bool ok() const { return _type == NoError; }

        const std::string &message() const { return *_msg; }

        const char *what() const { return _msg->c_str(); }

        static Error success() { return Error(NoError); }

    protected:
        int _type;
        std::shared_ptr<std::string> _msg;

        LANGMGR_EXPORT static std::shared_ptr<std::string> defaultMessage(int type);
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_ERROR_H
