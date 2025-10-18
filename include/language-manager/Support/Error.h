#ifndef LANGUAGE_MANAGER_ERROR_H
#define LANGUAGE_MANAGER_ERROR_H

#include <memory>
#include <string>

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
        };

        inline Error() : Error(NoError) {}

        inline explicit Error(int type) : Error(static_cast<Type>(type)) {}

        inline Error(Type type) : _type(type), _msg(defaultMessage(type)) {}

        inline Error(int type, std::string msg) : _type(type), _msg(std::make_shared<std::string>(std::move(msg))) {}

        inline Error(int type, const char *msg) : _type(type), _msg(std::make_shared<std::string>(msg)) {}

        inline int type() const { return _type; }

        inline bool ok() const { return _type == NoError; }

        inline const std::string &message() const { return *_msg; }

        inline const char *what() const { return _msg->c_str(); }

        static Error success() { return Error(NoError); }

    protected:
        int _type;
        std::shared_ptr<std::string> _msg;

        static std::shared_ptr<std::string> defaultMessage(int type);
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_ERROR_H
